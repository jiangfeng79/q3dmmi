#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <map>
#include <stdexcept>

enum class Kind { Boolean, Integer, Float, Double, String, UInt64, Rect };

struct Field
{
    QString group;
    QString key;
    QString member;
    QString event;
    Kind kind;
    QJsonValue initial;
};

using UserBlocks = std::map<QString, QString>;

QString upperFirst(QString text)
{
    if (!text.isEmpty()) text[0] = text[0].toUpper();
    return text;
}

QString memberName(const QString& key)
{
    return key == QStringLiteral("mask") ? QStringLiteral("displayMask") : key;
}

QString quoted(const QString& text)
{
    QString escaped = text;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QStringLiteral("QStringLiteral(\"") + escaped + QStringLiteral("\")");
}

Kind fieldKind(const QString& key, const QJsonValue& value)
{
    if (value.isBool()) return Kind::Boolean;
    if (value.isArray()) return Kind::Rect;
    if (value.isString() && key == QStringLiteral("mask")) return Kind::UInt64;
    if (value.isString()) return Kind::String;
    if (key == QStringLiteral("mapOpMode")) return Kind::Integer;
    if (key == QStringLiteral("rotationAngle")) return Kind::Double;
    return Kind::Float;
}

QString cppType(Kind kind)
{
    switch (kind)
    {
        case Kind::Boolean: return QStringLiteral("bool");
        case Kind::Integer: return QStringLiteral("int");
        case Kind::Float: return QStringLiteral("float");
        case Kind::Double: return QStringLiteral("double");
        case Kind::String: return QStringLiteral("QString");
        case Kind::UInt64: return QStringLiteral("std::uint64_t");
        case Kind::Rect: return QStringLiteral("QRect");
    }
    return {};
}

QString initialValue(const Field& field)
{
    switch (field.kind)
    {
        case Kind::Boolean: return field.initial.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        case Kind::Integer: return QString::number(field.initial.toInt());
        case Kind::Float:
        {
            QString value = QString::number(field.initial.toDouble(), 'g', 9);
            if (!value.contains(QLatin1Char('.')) && !value.contains(QLatin1Char('e'), Qt::CaseInsensitive))
                value += QStringLiteral(".0");
            return value + QLatin1Char('f');
        }
        case Kind::Double: return QString::number(field.initial.toDouble(), 'g', 17);
        case Kind::String: return quoted(field.initial.toString());
        case Kind::UInt64:
        {
            bool ok = false;
            const qulonglong value = field.initial.toString().toULongLong(&ok, 0);
            if (!ok) throw std::runtime_error("Invalid hexadecimal uint64 field");
            return QStringLiteral("0x%1ULL").arg(value, 16, 16, QLatin1Char('0'));
        }
        case Kind::Rect:
        {
            const QJsonArray values = field.initial.toArray();
            if (values.size() != 4) throw std::runtime_error("Rectangle fields require four values");
            return QStringLiteral("QRect(%1, %2, %3, %4)")
                .arg(values[0].toInt()).arg(values[1].toInt()).arg(values[2].toInt()).arg(values[3].toInt());
        }
    }
    return {};
}

UserBlocks readUserBlocks(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    const QString text = QString::fromUtf8(file.readAll());
    UserBlocks blocks;
    const QString startPrefix = QStringLiteral("// <user-code: ");
    qsizetype position = 0;
    while ((position = text.indexOf(startPrefix, position)) >= 0)
    {
        const qsizetype nameStart = position + startPrefix.size();
        const qsizetype nameEnd = text.indexOf(QLatin1Char('>'), nameStart);
        if (nameEnd < 0) break;
        const QString name = text.mid(nameStart, nameEnd - nameStart);
        const QString endMarker = QStringLiteral("// </user-code: %1>").arg(name);
        const qsizetype codeStart = text.indexOf(QLatin1Char('\n'), nameEnd) + 1;
        const qsizetype codeEnd = text.indexOf(endMarker, codeStart);
        if (codeStart <= 0 || codeEnd < 0) break;
        QString code = text.mid(codeStart, codeEnd - codeStart);
        while (code.endsWith(QLatin1Char(' ')) || code.endsWith(QLatin1Char('\t'))) code.chop(1);
        blocks.emplace(name, code);
        position = codeEnd + endMarker.size();
    }
    return blocks;
}

void writeFile(const QString& path, const QString& content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        throw std::runtime_error(QStringLiteral("Cannot write %1").arg(path).toStdString());
    file.write(content.toUtf8());
}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (argc != 4)
    {
        qCritical("Usage: app_config_codegen <config.json> <output-directory> <events.cpp>");
        return 1;
    }

    QFile schemaFile(QString::fromLocal8Bit(argv[1]));
    if (!schemaFile.open(QIODevice::ReadOnly)) return 1;
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(schemaFile.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return 1;

    QVector<Field> fields;
    const QJsonObject root = document.object();
    for (auto groupIt = root.begin(); groupIt != root.end(); ++groupIt)
    {
        if (!groupIt.value().isObject()) continue;
        const QJsonObject group = groupIt.value().toObject();
        for (auto fieldIt = group.begin(); fieldIt != group.end(); ++fieldIt)
        {
            const QString member = memberName(fieldIt.key());
            fields.push_back({groupIt.key(), fieldIt.key(), member, upperFirst(member),
                              fieldKind(fieldIt.key(), fieldIt.value()), fieldIt.value()});
        }
    }
    if (fields.isEmpty()) return 1;

    const QString outputDirectory = QString::fromLocal8Bit(argv[2]);
    QDir().mkpath(outputDirectory);
    QString header;
    QTextStream h(&header);
    h << "#pragma once\n\n#include <QRect>\n#include <QString>\n#include <cstdint>\n#include <functional>\n"
         "#include <map>\n#include <vector>\n\nclass AppConfig\n{\npublic:\n    struct Data\n    {\n";
    for (const Field& field : fields)
        h << "        " << cppType(field.kind) << ' ' << field.member << " = " << initialValue(field) << ";\n";
    h << "    };\n\n    enum class Field\n    {\n";
    for (const Field& field : fields) h << "        " << field.event << ",\n";
    h << "    };\n\n    class Database\n    {\n    public:\n"
         "        using Observer = std::function<void(Field, const Data&)>;\n"
         "        using ObserverId = std::size_t;\n\n"
         "        const Data& data() const;\n        void replace(const Data& data);\n"
         "        bool load(const QString& path);\n        bool save(const QString& path) const;\n"
         "        ObserverId addObserver(Observer observer);\n        void removeObserver(ObserverId observerId);\n"
         "    private:\n        void notify(Field field);\n        Data m_data;\n"
         "        std::map<ObserverId, Observer> m_observers;\n        ObserverId m_nextObserverId = 1;\n    };\n\n"
         "    static QString configPath();\n    static bool load(const QString& path, Data& out);\n"
         "    static bool save(const QString& path, const Data& in);\n"
         "    static const std::vector<Field>& fields();\n    static QString fieldGroup(Field field);\n"
         "    static QString fieldName(Field field);\n    static QString fieldValue(Field field, const Data& data);\n"
         "    static bool setFieldValue(Data& data, Field field, const QString& text);\n};\n\n";
    for (const Field& field : fields) h << "void on" << field.event << "Changed(AppConfig::Data& data);\n";

    QString source;
    QTextStream s(&source);
    s << "#include \"appConfig.h\"\n\n#include <QCoreApplication>\n#include <QDebug>\n#include <QDir>\n"
         "#include <QFile>\n#include <QFileInfo>\n#include <QJsonArray>\n#include <QJsonDocument>\n"
         "#include <QJsonObject>\n#include <limits>\n#include <utility>\n\n"
         "const AppConfig::Data& AppConfig::Database::data() const { return m_data; }\n\n"
         "void AppConfig::Database::replace(const Data& data)\n{\n    const Data previous = m_data;\n    m_data = data;\n";
    for (const Field& field : fields)
        s << "    if (previous." << field.member << " != data." << field.member << ") notify(Field::" << field.event << ");\n";
    s << "}\n\nbool AppConfig::Database::load(const QString& path)\n{\n    Data loaded = m_data;\n"
         "    if (!AppConfig::load(path, loaded)) return false;\n    m_data = loaded;\n"
         "    for (Field field : AppConfig::fields()) notify(field);\n    return true;\n}\n\n"
         "bool AppConfig::Database::save(const QString& path) const { return AppConfig::save(path, m_data); }\n\n"
         "AppConfig::Database::ObserverId AppConfig::Database::addObserver(Observer observer)\n{\n"
         "    const ObserverId id = m_nextObserverId++;\n    m_observers.emplace(id, std::move(observer));\n    return id;\n}\n\n"
         "void AppConfig::Database::removeObserver(ObserverId id) { m_observers.erase(id); }\n\n"
         "void AppConfig::Database::notify(Field field)\n{\n    switch (field)\n    {\n";
    for (const Field& field : fields)
        s << "        case Field::" << field.event << ": on" << field.event << "Changed(m_data); break;\n";
    s << "    }\n    const auto observers = m_observers;\n    for (const auto& entry : observers)\n"
         "        if (entry.second) entry.second(field, m_data);\n}\n\n"
         "QString AppConfig::configPath() { return QCoreApplication::applicationDirPath() + QStringLiteral(\"/config.json\"); }\n\n"
         "bool AppConfig::load(const QString& path, Data& out)\n{\n    QFile file(path);\n"
         "    if (!file.open(QIODevice::ReadOnly)) return false;\n    QJsonParseError error{};\n"
         "    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);\n"
         "    if (error.error != QJsonParseError::NoError || !document.isObject()) return false;\n"
         "    const QJsonObject root = document.object();\n";
    QString currentGroup;
    for (const Field& field : fields)
    {
        if (field.group != currentGroup)
        {
            currentGroup = field.group;
            s << "    const QJsonObject " << currentGroup << " = root.value(" << quoted(currentGroup) << ").toObject();\n";
        }
        const QString value = field.group + QStringLiteral(".value(") + quoted(field.key) + QLatin1Char(')');
        switch (field.kind)
        {
            case Kind::Boolean: s << "    out." << field.member << " = " << value << ".toBool(out." << field.member << ");\n"; break;
            case Kind::Integer: s << "    out." << field.member << " = " << value << ".toInt(out." << field.member << ");\n"; break;
            case Kind::Float: s << "    out." << field.member << " = static_cast<float>(" << value << ".toDouble(out." << field.member << "));\n"; break;
            case Kind::Double: s << "    out." << field.member << " = " << value << ".toDouble(out." << field.member << ");\n"; break;
            case Kind::String: s << "    out." << field.member << " = " << value << ".toString(out." << field.member << ");\n"; break;
            case Kind::UInt64: s << "    { bool ok = false; const auto parsed = " << value << ".toString().toULongLong(&ok, 0); if (ok) out." << field.member << " = parsed; }\n"; break;
            case Kind::Rect: s << "    { const QJsonArray values = " << value << ".toArray(); if (values.size() == 4) out." << field.member << " = QRect(values[0].toInt(), values[1].toInt(), values[2].toInt(), values[3].toInt()); }\n"; break;
        }
    }
    s << "    return true;\n}\n\nbool AppConfig::save(const QString& path, const Data& in)\n{\n";
    currentGroup.clear();
    for (const Field& field : fields)
    {
        if (field.group != currentGroup)
        {
            currentGroup = field.group;
            s << "    QJsonObject " << currentGroup << ";\n";
        }
        if (field.kind == Kind::UInt64)
            s << "    " << field.group << '[' << quoted(field.key) << "] = QStringLiteral(\"0x%1\").arg(in." << field.member << ", 16, 16, QLatin1Char('0'));\n";
        else if (field.kind == Kind::Rect)
            s << "    " << field.group << '[' << quoted(field.key) << "] = QJsonArray{in." << field.member << ".x(), in." << field.member << ".y(), in." << field.member << ".width(), in." << field.member << ".height()};\n";
        else
            s << "    " << field.group << '[' << quoted(field.key) << "] = in." << field.member << ";\n";
    }
    s << "    QJsonObject root;\n";
    currentGroup.clear();
    for (const Field& field : fields) if (field.group != currentGroup) { currentGroup = field.group; s << "    root[" << quoted(currentGroup) << "] = " << currentGroup << ";\n"; }
    s << "    const QFileInfo info(path); QDir directory = info.absoluteDir();\n"
         "    if (!directory.exists() && !directory.mkpath(QStringLiteral(\".\"))) return false;\n"
         "    const QString temporaryPath = path + QStringLiteral(\".tmp\"); QFile file(temporaryPath);\n"
         "    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;\n"
         "    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) { file.close(); QFile::remove(temporaryPath); return false; }\n"
         "    file.close(); QFile::remove(path);\n    if (QFile::rename(temporaryPath, path)) return true;\n"
         "    QFile direct(path); if (!direct.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;\n"
         "    const bool ok = direct.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) >= 0; QFile::remove(temporaryPath); return ok;\n}\n\n"
         "const std::vector<AppConfig::Field>& AppConfig::fields()\n{\n    static const std::vector<Field> values = {";
    for (const Field& field : fields) s << "Field::" << field.event << ',';
    s << "}; return values;\n}\n\nQString AppConfig::fieldGroup(Field field)\n{\n    switch (field)\n    {\n";
    for (const Field& field : fields) s << "        case Field::" << field.event << ": return " << quoted(field.group) << ";\n";
    s << "    } return {};\n}\n\nQString AppConfig::fieldName(Field field)\n{\n    switch (field)\n    {\n";
    for (const Field& field : fields) s << "        case Field::" << field.event << ": return " << quoted(field.key) << ";\n";
    s << "    } return {};\n}\n\nQString AppConfig::fieldValue(Field field, const Data& data)\n{\n    switch (field)\n    {\n";
    for (const Field& field : fields)
    {
        s << "        case Field::" << field.event << ": ";
        if (field.kind == Kind::Boolean) s << "return data." << field.member << " ? QStringLiteral(\"true\") : QStringLiteral(\"false\");\n";
        else if (field.kind == Kind::UInt64) s << "return QStringLiteral(\"0x%1\").arg(data." << field.member << ", 16, 16, QLatin1Char('0'));\n";
        else if (field.kind == Kind::String) s << "return data." << field.member << ";\n";
        else if (field.kind == Kind::Rect) s << "return QStringLiteral(\"%1,%2,%3,%4\").arg(data." << field.member << ".x()).arg(data." << field.member << ".y()).arg(data." << field.member << ".width()).arg(data." << field.member << ".height());\n";
        else if (field.kind == Kind::Float) s << "return QString::number(data." << field.member << ", 'g', std::numeric_limits<float>::max_digits10);\n";
        else if (field.kind == Kind::Double) s << "return QString::number(data." << field.member << ", 'g', std::numeric_limits<double>::max_digits10);\n";
        else s << "return QString::number(data." << field.member << ");\n";
    }
    s << "    } return {};\n}\n\nbool AppConfig::setFieldValue(Data& data, Field field, const QString& text)\n{\n    bool ok = true;\n    switch (field)\n    {\n";
    for (const Field& field : fields)
    {
        s << "        case Field::" << field.event << ": ";
        if (field.kind == Kind::Boolean) s << "if (text.compare(QStringLiteral(\"true\"), Qt::CaseInsensitive) == 0 || text == QStringLiteral(\"1\")) data." << field.member << " = true; else if (text.compare(QStringLiteral(\"false\"), Qt::CaseInsensitive) == 0 || text == QStringLiteral(\"0\")) data." << field.member << " = false; else ok = false; break;\n";
        else if (field.kind == Kind::Integer) s << "data." << field.member << " = text.toInt(&ok); break;\n";
        else if (field.kind == Kind::Float) s << "data." << field.member << " = text.toFloat(&ok); break;\n";
        else if (field.kind == Kind::Double) s << "data." << field.member << " = text.toDouble(&ok); break;\n";
        else if (field.kind == Kind::UInt64) s << "data." << field.member << " = text.toULongLong(&ok, 0); break;\n";
        else if (field.kind == Kind::String) s << "data." << field.member << " = text; break;\n";
        else s << "{ const QStringList values = text.split(QLatin1Char(',')); int parts[4] = {}; ok = values.size() == 4; for (int index = 0; ok && index < 4; ++index) parts[index] = values[index].trimmed().toInt(&ok); if (ok) data." << field.member << " = QRect(parts[0], parts[1], parts[2], parts[3]); break; }\n";
    }
    s << "    } return ok;\n}\n";

    const QString viewHeader = QStringLiteral(
        "#pragma once\n#include <QWidget>\n#include \"appConfig.h\"\nclass QTreeWidget; class QTreeWidgetItem;\n"
        "class AppConfigView : public QWidget { public: explicit AppConfigView(AppConfig::Database&, QWidget* parent = nullptr);\n"
        "private: void updateItem(AppConfig::Field, const AppConfig::Data&); void commitItem(QTreeWidgetItem*);\n"
        "AppConfig::Database& m_database; QTreeWidget* m_tree; bool m_updating = false; };\n");
    const QString viewSource = QStringLiteral(R"(#include "appConfigView.h"
#include <QPointer>
#include <QTreeWidget>
#include <QVBoxLayout>

AppConfigView::AppConfigView(AppConfig::Database& database, QWidget* parent)
    : QWidget(parent), m_database(database), m_tree(new QTreeWidget(this))
{
    m_tree->setHeaderLabels({tr("Item"), tr("Value")});
    auto* layout = new QVBoxLayout(this); layout->setContentsMargins(0, 0, 0, 0); layout->addWidget(m_tree);
    std::map<QString, QTreeWidgetItem*> branches;
    for (AppConfig::Field field : AppConfig::fields()) {
        const QString group = AppConfig::fieldGroup(field);
        if (!branches.contains(group)) branches[group] = new QTreeWidgetItem(m_tree, {group});
        auto* item = new QTreeWidgetItem(branches[group], {AppConfig::fieldName(field), AppConfig::fieldValue(field, database.data())});
        item->setData(0, Qt::UserRole, static_cast<int>(field)); item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    m_tree->expandAll();
    connect(m_tree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem* item, int column) { if (!m_updating && column == 1) commitItem(item); });
    QPointer<AppConfigView> guard(this);
    database.addObserver([guard](AppConfig::Field field, const AppConfig::Data& data) { if (guard) guard->updateItem(field, data); });
}

void AppConfigView::updateItem(AppConfig::Field field, const AppConfig::Data& data)
{
    m_updating = true;
    const auto items = m_tree->findItems(AppConfig::fieldName(field), Qt::MatchExactly | Qt::MatchRecursive, 0);
    if (!items.isEmpty()) items.first()->setText(1, AppConfig::fieldValue(field, data));
    m_updating = false;
}

void AppConfigView::commitItem(QTreeWidgetItem* item)
{
    if (!item->parent()) return;
    const auto field = static_cast<AppConfig::Field>(item->data(0, Qt::UserRole).toInt());
    AppConfig::Data data = m_database.data();
    if (AppConfig::setFieldValue(data, field, item->text(1).trimmed())) m_database.replace(data);
    else updateItem(field, m_database.data());
}
)");

    const QString eventsPath = QString::fromLocal8Bit(argv[3]);
    const UserBlocks blocks = readUserBlocks(eventsPath);
    QString events = QStringLiteral("#include \"appConfig.h\"\n// <user-code: headers>\n");
    if (const auto it = blocks.find(QStringLiteral("headers")); it != blocks.end()) events += it->second;
    events += QStringLiteral("// </user-code: headers>\n\n");
    for (const Field& field : fields)
    {
        events += QStringLiteral("void on%1Changed(AppConfig::Data& data)\n{\n    // <user-code: %1>\n").arg(field.event);
        if (const auto it = blocks.find(field.event); it != blocks.end()) events += it->second;
        else events += QStringLiteral("    // Review hmiios2014.cpp for runtime handling of this config field.\n    (void)data;\n");
        events += QStringLiteral("    // </user-code: %1>\n}\n\n").arg(field.event);
    }

    writeFile(outputDirectory + QStringLiteral("/appConfig.h"), header);
    writeFile(outputDirectory + QStringLiteral("/appConfig.cpp"), source);
    writeFile(outputDirectory + QStringLiteral("/appConfigView.h"), viewHeader);
    writeFile(outputDirectory + QStringLiteral("/appConfigView.cpp"), viewSource);
    writeFile(eventsPath, events);
    return 0;
}