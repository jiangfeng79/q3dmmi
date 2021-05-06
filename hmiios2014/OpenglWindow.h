#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include <QWindow>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFont>

QT_BEGIN_NAMESPACE
class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;
QT_END_NAMESPACE

//! [1]
class OpenglWindow : public QWindow, protected QOpenGLFunctions
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

public slots:
    void renderLater();
    void renderNow();
    void calculateFPS();
    void resetGeometry(QRect a_rect);

protected:
    bool event(QEvent* event);
    virtual void selectShader(uint shaderId);
    void exposeEvent(QExposeEvent* event);
    QTimer* timer;
    int m_fpsCounter, m_fps;
    int m_iMousePosX, m_iMousePosY;
    int m_iMouseInitX, m_iMouseInitY;
    int m_iMouseDeltaX, m_iMouseDeltaY;

    float m_fMapCenterDeltaX, m_fMapCenterDeltaY; // in meter
    float m_fMapPrevCenterDeltaX, m_fMapPrevCenterDeltaY; // in meter
    float m_fMotionSpeed, m_fMotionDir, m_fMousePressTime;

    double m_dRotationAngle, m_dPrevRotationAngle;
    bool m_bMouseIsPressing;
    float m_fScaleFactor;
    bool m_windowMaximized;
    QOpenGLContext* m_context;
    QOpenGLPaintDevice* m_device;
    unsigned int  m_uiMapOpMask;

    QFont m_font;

private:
    bool m_update_pending;
    bool m_animating;

signals:
    void signal_setFps(int a_iFps);

};
//! [1]

#endif // OPENGLWINDOW_H
