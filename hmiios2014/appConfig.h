#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QRect>
#include <QString>

#include <cstdint>

// Holds the user-configurable application state that is persisted to a JSON
// file (config.json, next to the executable) so it can be restored on the
// next launch. The struct is seeded with the current window state before
// load() is called, so any field absent from the JSON keeps its default.
class AppConfig
{
public:
    struct Data
    {
        // Camera / view state (OpenglWindow / TSDWindow).
        float centerX = 0.0f;
        float centerY = 0.0f;  // map center offset, meters
        float scale = 1.0f;    // zoom level
        double rotationAngle = 0.0;

        // Display state (TSDWindow).
        std::uint64_t displayMask = 0;
        bool autoZoom = false;
        bool autoSwing = false;
        bool shaderToys = false;
        bool vsync = true;
        int mapOpMode = 1;  // OpenglWindow::MapOpMaskBits (PAN=1, EBL=2)

        // Application state.
        QString language = QStringLiteral("en");
        QRect windowGeometry;  // empty/invalid = use default size
        bool maximized = false;
    };

    // Path used for the config file: config.json next to the executable.
    static QString configPath();

    // Load config from disk into `out`. Only fields present in the JSON are
    // overwritten; absent fields keep whatever value `out` already holds.
    // Returns false if the file does not exist or cannot be parsed (in which
    // case `out` is left untouched).
    static bool load(const QString& path, Data& out);

    // Save config to disk atomically. Returns false on write failure.
    static bool save(const QString& path, const Data& in);
};

#endif  // APPCONFIG_H
