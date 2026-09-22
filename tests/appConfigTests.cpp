#include "appConfig.h"

#include <QTemporaryDir>

#include <set>

int main()
{
    QTemporaryDir directory;
    if (!directory.isValid())
    {
        return 1;
    }

    AppConfig::Database database;
    std::set<AppConfig::Field> changes;
    const auto observerId = database.addObserver(
        [&changes](AppConfig::Field field, const AppConfig::Data&) { changes.insert(field); });

    if (!database.load(QStringLiteral(APP_CONFIG_SCHEMA_PATH)) || changes.size() != AppConfig::fields().size())
    {
        return 2;
    }

    const AppConfig::Data& data = database.data();
    for (AppConfig::Field field : AppConfig::fields())
    {
        if (AppConfig::fieldGroup(field).isEmpty() || AppConfig::fieldName(field).isEmpty() ||
            AppConfig::fieldValue(field, data).isEmpty())
        {
            return 3;
        }
    }

    database.removeObserver(observerId);

    AppConfig::Data cameraUpdate = database.data();
    bool hasCenterX = false;
    bool hasCenterY = false;
    for (AppConfig::Field field : AppConfig::fields())
    {
        if (AppConfig::fieldGroup(field) != QStringLiteral("camera")) continue;
        if (AppConfig::fieldName(field) == QStringLiteral("centerX"))
            hasCenterX = AppConfig::setFieldValue(cameraUpdate, field, QStringLiteral("0.833333313"));
        else if (AppConfig::fieldName(field) == QStringLiteral("centerY"))
            hasCenterY = AppConfig::setFieldValue(cameraUpdate, field, QStringLiteral("1.66666663"));
    }
    if (hasCenterX && hasCenterY)
    {
        bool consistentCameraSnapshot = true;
        const auto cameraObserverId = database.addObserver(
            [&consistentCameraSnapshot](AppConfig::Field, const AppConfig::Data& snapshot) {
                QString centerX;
                QString centerY;
                for (AppConfig::Field field : AppConfig::fields())
                {
                    if (AppConfig::fieldGroup(field) != QStringLiteral("camera")) continue;
                    if (AppConfig::fieldName(field) == QStringLiteral("centerX"))
                        centerX = AppConfig::fieldValue(field, snapshot);
                    else if (AppConfig::fieldName(field) == QStringLiteral("centerY"))
                        centerY = AppConfig::fieldValue(field, snapshot);
                }
                consistentCameraSnapshot = consistentCameraSnapshot && centerX == QStringLiteral("0.833333313") &&
                                           centerY == QStringLiteral("1.66666663");
            });
        database.replace(cameraUpdate);
        database.removeObserver(cameraObserverId);
        if (!consistentCameraSnapshot)
        {
            return 4;
        }
    }

    AppConfig::Data fullscreenUpdate = database.data();
    bool hasFullscreen = false;
    for (AppConfig::Field field : AppConfig::fields())
    {
        if (AppConfig::fieldGroup(field) == QStringLiteral("app") &&
            AppConfig::fieldName(field) == QStringLiteral("fullscreen"))
        {
            hasFullscreen = AppConfig::setFieldValue(fullscreenUpdate, field, QStringLiteral("true"));
            break;
        }
    }
    if (hasFullscreen)
    {
        database.replace(fullscreenUpdate);
    }

    database.replace(data);
    if (changes.size() != AppConfig::fields().size())
    {
        return 5;
    }

    const QString outputPath = directory.filePath(QStringLiteral("saved.json"));
    AppConfig::Data roundTrip;
    if (!database.save(outputPath) || !AppConfig::load(outputPath, roundTrip))
    {
        return 6;
    }
    for (AppConfig::Field field : AppConfig::fields())
    {
        if (AppConfig::fieldValue(field, roundTrip) != AppConfig::fieldValue(field, data))
        {
            return 7;
        }
        if (hasFullscreen && AppConfig::fieldGroup(field) == QStringLiteral("app") &&
            AppConfig::fieldName(field) == QStringLiteral("fullscreen") &&
            AppConfig::fieldValue(field, roundTrip) != QStringLiteral("true"))
        {
            return 8;
        }
    }

    return 0;
}