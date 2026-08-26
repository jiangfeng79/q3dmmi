#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include <QElapsedTimer>
#include <QFont>
#include <QFontMetricsF>
#include <QHash>
#include <QMouseEvent>
#include <QOpenGLFunctions_3_3_Core>
#include <QTimer>
#include <QWheelEvent>
#include <QWindow>

QT_BEGIN_NAMESPACE
class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;
QT_END_NAMESPACE

//! [1]
class OpenglWindow : public QWindow, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    enum MapOpMaskBits
    {
        PAN = 1,
        EBL = 1 << 1
    };

    explicit OpenglWindow(QWindow* parent = 0);
    ~OpenglWindow();

    virtual void render(QPainter* painter);
    virtual void render();

    virtual void initialize();

    void setAnimating(bool animating);
    void renderText(int posX, int posY, const QString& text);
    void renderText(int posX, int posY, const QString& text, const QString& font);
    void renderText(int posX, int posY, const QString& text, const QString& font, qreal angle);
    void renderShape(const QRect& rec);
    void drawLines(const QVector<QPointF>& pointPairs);

    void setMapOpMask(MapOpMaskBits layer);

    // Vsync (benchmark) toggle. The swap interval is a GLX context-creation
    // attribute, so changing it at runtime requires destroying and recreating
    // the current context; renderNow() rebuilds it lazily on the next frame.
    void toggleVsync();
    bool vsyncEnabled() const { return m_bVsyncEnabled; }

public slots:
    void renderLater();
    void renderNow();
    void calculateFPS();
    void resetGeometry(QRect a_rect);

protected:
    bool event(QEvent* event);
    virtual void selectShader(uint shaderId);
    // Called when the GL context is destroyed at runtime (e.g. vsync toggle).
    // Subclasses must release their CPU-side data here so initialize() can run
    // again cleanly on the recreated context. Default: nothing to do.
    virtual void resetGpuResources();
    void exposeEvent(QExposeEvent* event);
    QTimer* timer;
    int m_fpsCounter, m_fps;
    int m_iMousePosX, m_iMousePosY;
    int m_iMouseInitX, m_iMouseInitY;
    int m_iMouseDeltaX, m_iMouseDeltaY;

    float m_fMapCenterDeltaX, m_fMapCenterDeltaY;          // in meter
    float m_fMapPrevCenterDeltaX, m_fMapPrevCenterDeltaY;  // in meter
    float m_fMotionSpeed, m_fMotionDir;
    QElapsedTimer m_mousePressTimer;

    double m_dRotationAngle, m_dPrevRotationAngle;
    bool m_bMouseIsPressing;
    float m_fScaleFactor;
    bool m_windowMaximized;
    QOpenGLContext* m_context;
    // Runtime vsync state. true = swap interval 1 (vsync on, ~60 Hz cap),
    // false = swap interval 0 (benchmark mode, unlimited FPS).
    bool m_bVsyncEnabled;
    QOpenGLPaintDevice* m_device;
    unsigned int m_uiMapOpMask;

    QFont m_font;
    bool m_inTextFrame;

    // Batched 2D text pass. The device is reused across a whole frame so we
    // only pay the QOpenGLPaintDevice/QPainter setup cost once instead of once
    // per label. Exposed as protected so subclasses (e.g. TSDWindow) can draw
    // into the same batched 2D text pass for dynamic overlays such as live
    // flights.
    QOpenGLPaintDevice* m_batchDevice;
    QPainter* m_batchPainter;

    // Returns an active QPainter: the shared batch painter if one is active,
    // otherwise a freshly begun one (caller must end() it when !shared).
    QPainter* activeTextPainter(bool& shared);

    // Font cache for the hot label-draw path. Building a QFont (family string
    // parse) and a QFontMetricsF per label is expensive when there are tens of
    // thousands of labels per frame; these are keyed by family/size/bold and
    // reused, so drawText() only does layout + the glyph upload.
    struct FontEntry
    {
        FontEntry() = delete;
        FontEntry(const QFont& f, const QFontMetricsF& m) : font(f), metrics(m) {}
        QFont font;
        QFontMetricsF metrics;
    };
    QHash<QString, FontEntry> m_fontCache;
    FontEntry* getCachedFont(const QString& family, int pixelSize, bool bold);

private:
    bool m_update_pending;
    bool m_animating;

signals:
    void signal_setFps(int a_iFps);
};
//! [1]

#endif  // OPENGLWINDOW_H
