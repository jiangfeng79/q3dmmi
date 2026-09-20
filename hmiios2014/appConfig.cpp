#include "appConfig.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

QString AppConfig::configPath()
{
    // Store the config next to the executable so it travels with the build
    // output (the same place sgMap/ and datamall.secret live).
    return QCoreApplication::applicationDirPath() + QStringLiteral("/config.json");
}

bool AppConfig::load(const QString& path, Data& out)
{
    QFile file(path);
    if (!file.exists())
    {
        return false;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "AppConfig: cannot open" << path;
        return false;
    }

    const QByteArray raw = file.readAll();
    file.close();

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        qWarning() << "AppConfig: failed to parse" << path << parseError.errorString();
        return false;
    }

    const QJsonObject root = doc.object();

    // Camera / view state.
    if (root.contains(QStringLiteral("camera")))
    {
        const QJsonObject cam = root.value(QStringLiteral("camera")).toObject();
        out.centerX = static_cast<float>(cam.value(QStringLiteral("centerX")).toDouble(out.centerX));
        out.centerY = static_cast<float>(cam.value(QStringLiteral("centerY")).toDouble(out.centerY));
        out.scale = static_cast<float>(cam.value(QStringLiteral("scale")).toDouble(out.scale));
        out.rotationAngle = cam.value(QStringLiteral("rotationAngle")).toDouble(out.rotationAngle);
    }

    // Display state.
    if (root.contains(QStringLiteral("display")))
    {
        const QJsonObject disp = root.value(QStringLiteral("display")).toObject();
        if (disp.contains(QStringLiteral("mask")))
        {
            // Stored as a hex string ("0x...") to keep the 64-bit value exact.
            bool ok = false;
            const std::uint64_t mask = disp.value(QStringLiteral("mask")).toString().toULongLong(&ok, 16);
            if (ok)
            {
                out.displayMask = mask;
            }
        }
        out.autoZoom = disp.value(QStringLiteral("autoZoom")).toBool(out.autoZoom);
        out.autoSwing = disp.value(QStringLiteral("autoSwing")).toBool(out.autoSwing);
        out.shaderToys = disp.value(QStringLiteral("shaderToys")).toBool(out.shaderToys);
        out.vsync = disp.value(QStringLiteral("vsync")).toBool(out.vsync);
        out.mapOpMode = static_cast<int>(disp.value(QStringLiteral("mapOpMode")).toInt(out.mapOpMode));
    }

    // Application state.
    if (root.contains(QStringLiteral("app")))
    {
        const QJsonObject app = root.value(QStringLiteral("app")).toObject();
        out.language = app.value(QStringLiteral("language")).toString(out.language);
        out.maximized = app.value(QStringLiteral("maximized")).toBool(out.maximized);

        const QJsonArray geom = app.value(QStringLiteral("windowGeometry")).toArray();
        if (geom.size() == 4)
        {
            out.windowGeometry = QRect(static_cast<int>(geom.at(0).toInt()), static_cast<int>(geom.at(1).toInt()),
                                       static_cast<int>(geom.at(2).toInt()), static_cast<int>(geom.at(3).toInt()));
        }
    }

    return true;
}

bool AppConfig::save(const QString& path, const Data& in)
{
    QJsonObject cam;
    cam[QStringLiteral("centerX")] = in.centerX;
    cam[QStringLiteral("centerY")] = in.centerY;
    cam[QStringLiteral("scale")] = in.scale;
    cam[QStringLiteral("rotationAngle")] = in.rotationAngle;

    QJsonObject disp;
    disp[QStringLiteral("mask")] = QStringLiteral("0x%1").arg(in.displayMask, 16, 16, QLatin1Char('0'));
    disp[QStringLiteral("autoZoom")] = in.autoZoom;
    disp[QStringLiteral("autoSwing")] = in.autoSwing;
    disp[QStringLiteral("shaderToys")] = in.shaderToys;
    disp[QStringLiteral("vsync")] = in.vsync;
    disp[QStringLiteral("mapOpMode")] = in.mapOpMode;

    QJsonObject app;
    app[QStringLiteral("language")] = in.language;
    app[QStringLiteral("maximized")] = in.maximized;
    if (in.windowGeometry.isValid())
    {
        QJsonArray geom;
        geom.append(in.windowGeometry.x());
        geom.append(in.windowGeometry.y());
        geom.append(in.windowGeometry.width());
        geom.append(in.windowGeometry.height());
        app[QStringLiteral("windowGeometry")] = geom;
    }

    QJsonObject root;
    root[QStringLiteral("camera")] = cam;
    root[QStringLiteral("display")] = disp;
    root[QStringLiteral("app")] = app;

    // Write atomically: write to a temp file in the same directory, then
    // replace, so a crash mid-write never corrupts the existing config.
    const QFileInfo info(path);
    QDir dir = info.absoluteDir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
    {
        qWarning() << "AppConfig: cannot create directory" << dir.absolutePath();
        return false;
    }

    const QString tmpPath = path + QStringLiteral(".tmp");
    QFile file(tmpPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "AppConfig: cannot open" << tmpPath << "for writing";
        return false;
    }
    const bool wrote = file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    if (!wrote)
    {
        qWarning() << "AppConfig: failed to write" << tmpPath;
        QFile::remove(tmpPath);
        return false;
    }

    QFile::remove(path);
    if (!QFile::rename(tmpPath, path))
    {
        // Fall back to a plain copy if the atomic rename is not possible.
        qWarning() << "AppConfig: atomic rename failed, falling back to direct write";
        QFile direct(path);
        if (direct.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            const bool ok = direct.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
            direct.close();
            QFile::remove(tmpPath);
            return ok;
        }
        return false;
    }

    return true;
}
