#include "OpenglWindow.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFontMetricsF>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QPainter>

#include <math.h>
#include <time.h>

#define MOTION_TIMEOUT 20
//! [1]
OpenglWindow::OpenglWindow(QWindow* parent)
    : QWindow(parent),
      m_update_pending(false),
      m_animating(false),
      m_context(0),
      m_device(0),
      m_batchDevice(nullptr),
      m_batchPainter(nullptr),
      m_inTextFrame(false),
      m_fps(0),
      m_fpsCounter(0),
      m_iMousePosX(0),
      m_iMousePosY(0),
      m_iMouseInitX(0),
      m_iMouseInitY(0),
      m_iMouseDeltaX(0),
      m_iMouseDeltaY(0)
      //	, m_iCenterDeltaX(0)
      //	, m_iCenterDeltaY(0)
      ,
      m_fMapCenterDeltaX(0),
      m_fMapCenterDeltaY(0),
      m_fMapPrevCenterDeltaX(0),
      m_fMapPrevCenterDeltaY(0),
      m_dPrevRotationAngle(0),
      m_fMotionSpeed(0),
      m_fMotionDir(0),
      m_uiMapOpMask(PAN),
      m_fScaleFactor(1),
      m_bMouseIsPressing(false),
      m_windowMaximized(false)
      // Start with vsync ON (~60 Hz cap). The user can flip this at runtime via
      // the Debug > Vsync toggle menu item to enter benchmark mode (unlimited FPS).
      ,
      m_bVsyncEnabled(true)

{
    setSurfaceType(QWindow::OpenGLSurface);
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(calculateFPS()));
    timer->start(1000);
}
//! [1]

OpenglWindow::~OpenglWindow()
{
    delete timer;
    delete m_context;
    delete m_device;
    if (m_batchPainter)
    {
        m_batchPainter->end();
    }
    delete m_batchPainter;
    delete m_batchDevice;
}
//! [2]
void OpenglWindow::render(QPainter* painter)
{
    Q_UNUSED(painter);
}

void OpenglWindow::initialize() {}

void OpenglWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    QOpenGLPaintDevice device;
    device.setDevicePixelRatio(devicePixelRatio());
    device.setSize(size() * devicePixelRatio());

    QPainter painter(&device);
    render(&painter);
}
//! [2]

//! [3]
void OpenglWindow::renderLater()
{
    if (!m_update_pending)
    {
        m_update_pending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool OpenglWindow::event(QEvent* event)
{
    switch (event->type())
    {
        case QEvent::UpdateRequest: {
            m_update_pending = false;
            renderNow();
            return true;
        }
        case QEvent::MouseMove: {
            QMouseEvent* l_mouseEvent = static_cast<QMouseEvent*>(event);
            m_iMousePosX = qRound(l_mouseEvent->position().x());
            m_iMousePosY = qRound(l_mouseEvent->position().y());
            if (m_bMouseIsPressing)
            {
                m_iMouseDeltaX = m_iMousePosX - m_iMouseInitX;
                m_iMouseDeltaY = m_iMousePosY - m_iMouseInitY;
                if (l_mouseEvent->buttons() == Qt::RightButton)
                {
                    /*
                    //calculate angle
                    //qDebug() << "prev angle: " << m_dPrevRotationAngle << "angle:" <<m_dRotationAngle;
                    double a = sqrt(m_iMouseDeltaY*m_iMouseDeltaY+m_iMouseDeltaX*m_iMouseDeltaX);
                    double b = sqrt((m_iMousePosY-m_fMapCenterDeltaY)*(m_iMousePosY-m_fMapCenterDeltaY) +
                    (m_iMousePosX-m_fMapCenterDeltaX)*(m_iMousePosX-m_fMapCenterDeltaX)); double c =
                    sqrt((m_iMouseInitY-m_fMapCenterDeltaY)*(m_iMouseInitY-m_fMapCenterDeltaY) +
                    (m_iMouseInitX-m_fMapCenterDeltaX)*(m_iMouseInitX-m_fMapCenterDeltaX));
                    //b^2+c^2-a^2)/2bc
                    //m_dRotationAngle = m_dPrevRotationAngle+atan2(m_iMouseDeltaY,(m_iMouseInitX-m_fMapCenterDeltaX));
                    float y = (float)m_iMouseInitY*((float)m_iMousePosX -
                    m_fMapCenterDeltaX)-m_fMapCenterDeltaY*(float)m_iMouseDeltaX; float x =
                    (float)m_iMouseInitX-m_fMapCenterDeltaX; qDebug() << "y:" <<y << "x:" << x; if(y*x >0)
                    {

                        m_dRotationAngle = m_dPrevRotationAngle-acos((b*b+c*c-a*a)/(2*b*c));
                    }
                    else
                        m_dRotationAngle = m_dPrevRotationAngle+acos((b*b+c*c-a*a)/(2*b*c));
                    qDebug() << "prev angle: " << m_dPrevRotationAngle << "angle:" <<m_dRotationAngle;
                    //m_dPrevRotationAngle += m_dRotationAngle;
                    */
                }
            }
            if (m_uiMapOpMask == PAN && l_mouseEvent->buttons() & Qt::LeftButton)
            {
                m_fMapCenterDeltaX = m_fMapPrevCenterDeltaX + m_iMouseDeltaX / m_fScaleFactor;
                m_fMapCenterDeltaY = m_fMapPrevCenterDeltaY + m_iMouseDeltaY / m_fScaleFactor;
            }

            return true;
            // return QWindow::event(event);
        }
        case QEvent::MouseButtonPress: {
            QMouseEvent* l_mouseEvent = static_cast<QMouseEvent*>(event);
            if (l_mouseEvent->buttons() & Qt::LeftButton /*|| l_mouseEvent->buttons() == Qt::RightButton*/)
            {
                // qDebug() << "left click";
                m_iMouseInitX = qRound(l_mouseEvent->position().x());
                m_iMouseInitY = qRound(l_mouseEvent->position().y());
                m_iMouseDeltaX = 0;
                m_iMouseDeltaY = 0;
                m_bMouseIsPressing = true;

                m_fMotionSpeed = 0.0f;
                m_mousePressTimer.restart();
            }
            else if (l_mouseEvent->buttons() & Qt::RightButton)
            {
                // qDebug() << "right click";
                m_iMouseInitX = qRound(l_mouseEvent->position().x());
                m_iMouseInitY = qRound(l_mouseEvent->position().y());
                m_iMouseDeltaX = 0;
                m_iMouseDeltaY = 0;
                m_bMouseIsPressing = true;
                // m_dRotationAngle = 0;
            }
            return true;
        }
        case QEvent::MouseButtonRelease: {
            QMouseEvent* l_mouseEvent = static_cast<QMouseEvent*>(event);
            m_bMouseIsPressing = false;
            // m_iCenterDeltaX += m_iMouseDeltaX;
            // m_iCenterDeltaY += m_iMouseDeltaY;
            if (m_uiMapOpMask == PAN)
            {
                m_fMapPrevCenterDeltaX = m_fMapCenterDeltaX;
                m_fMapPrevCenterDeltaY = m_fMapCenterDeltaY;

                const qreal dx = qreal(m_iMouseDeltaX);
                const qreal dy = qreal(m_iMouseDeltaY);
                const qreal dt = m_mousePressTimer.isValid() ? m_mousePressTimer.elapsed() / 1000.0 : qreal(0.0);
                const qreal scaleFactor = qMax(qreal(m_fScaleFactor), qreal(1e-4));

                if (dt > 0.0 && dt < 0.2 && (dx != 0.0 || dy != 0.0))
                {
                    m_fMotionSpeed = float(std::hypot(dx, dy) / dt / scaleFactor);
                    m_fMotionDir = float(std::atan2(dy, dx));
                }
            }
            m_iMouseDeltaX = 0;
            m_iMouseDeltaY = 0;
            if (l_mouseEvent->button() == Qt::RightButton)
            {
                m_dPrevRotationAngle = m_dRotationAngle;
                // m_dRotationAngle = 0;
                qDebug() << "accumulate prev angle: " << m_dPrevRotationAngle;
            }
            return true;
        }
        case QEvent::Wheel: {
            QWheelEvent* l_wheelEvent = static_cast<QWheelEvent*>(event);
            int numDegrees = l_wheelEvent->angleDelta().y() / 8;
            int numSteps = numDegrees / 15;
            m_fScaleFactor *= pow(1.2, numSteps);
            if (m_fScaleFactor > 1600.0)
            {
                m_fScaleFactor = 1600.0;
            }
            if (m_fScaleFactor < .0001)
            {
                m_fScaleFactor = .0001;
            }

            return true;
        }
        case QEvent::KeyPress: {
            QKeyEvent* l_keyevent = static_cast<QKeyEvent*>(event);
            if (l_keyevent->key() == Qt::Key_F)
            {
                if (m_windowMaximized)
                {
                    showNormal();
                    m_windowMaximized = false;
                }
                else
                {
                    showFullScreen();
                    m_windowMaximized = true;
                }
            }
            else if (l_keyevent->key() == Qt::Key_Escape)
            {
                close();
            }
            else if (l_keyevent->key() == Qt::Key_1 || l_keyevent->key() == Qt::Key_2 || l_keyevent->key() == Qt::Key_3
                     || l_keyevent->key() == Qt::Key_4 || l_keyevent->key() == Qt::Key_5
                     || l_keyevent->key() == Qt::Key_6 || l_keyevent->key() == Qt::Key_7
                     || l_keyevent->key() == Qt::Key_8 || l_keyevent->key() == Qt::Key_9
                     || l_keyevent->key() == Qt::Key_0)
            {
                selectShader(l_keyevent->key() - Qt::Key_0);
            }
            return true;
        }
        default: return QWindow::event(event);
    }
}

void OpenglWindow::exposeEvent(QExposeEvent* event)
{
    Q_UNUSED(event);

    if (isExposed())
    {
        renderNow();
    }
}
//! [3]

//! [4]
void OpenglWindow::renderNow()
{
    if (!isExposed())
    {
        return;
    }

    bool needsInitialize = false;

    if (!m_context)
    {
        m_context = new QOpenGLContext(this);
        QSurfaceFormat format = requestedFormat();
        // Use the window's requested format directly; it is already configured
        // before the window is created and avoids redundant overrides.

        // The swap interval is a GLX *context-creation* attribute, so it must
        // be set here, before the context is created; it cannot be changed on
        // an existing context. SwapInterval(0) removes the per-frame wait for
        // the monitor's vertical refresh (~60 Hz), letting the render loop in
        // renderNow() run as fast as the GPU/CPU allow (benchmark mode).
        // m_bVsyncEnabled is toggled at runtime via Debug > Vsync toggle,
        // which destroys this context so it gets recreated with the new value.
        format.setSwapInterval(m_bVsyncEnabled ? 1 : 0);

        m_context->setFormat(format);
        // qDebug() << "Requested surface format:" << format;
        if (!m_context->create())
        {
            qWarning() << "QOpenGLContext::create() failed for window" << title();
            // m_context is a child of this QWindow, so it will be deleted with
            // the parent; only release the GLX resources here.
            delete m_context;
            m_context = nullptr;
            return;
        }
        // qDebug() << "Created context format:" << m_context->format() << "window format:" << requestedFormat();
        needsInitialize = true;
    }

    if (!m_context->makeCurrent(this))
    {
        qWarning() << "QOpenGLContext::makeCurrent() failed, error:" << m_context->format().profile();
        return;
    }

    // qDebug() << "Context made current";
    initializeOpenGLFunctions();

    if (needsInitialize)
    {
        // qDebug() << "Calling initialize()";
        initialize();
        // GLenum err = glGetError();
        // if (err != GL_NO_ERROR)
        //     qWarning() << "GL error after initialize:" << err;
    }

    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
    // qDebug() << "Calling render()";
    render();
    GLenum err = glGetError();
    // if (err != GL_NO_ERROR)
    //     qWarning() << "GL error after render:" << err;

    m_context->swapBuffers(this);
    // qDebug() << "swapBuffers returned";

    // not to starve the other window events like window move
    QCoreApplication::processEvents();

    if (m_animating)
    {
        renderLater();
    }
}
//! [4]

//! [5]
void OpenglWindow::setAnimating(bool animating)
{
    m_animating = animating;

    if (animating)
    {
        renderLater();
    }
}
//! [5]

void OpenglWindow::toggleVsync()
{
    m_bVsyncEnabled = !m_bVsyncEnabled;
    qDebug() << "Vsync" << (m_bVsyncEnabled ? "ON (~60 Hz)" : "OFF (benchmark, unlimited FPS)");

    // The swap interval is fixed at GLX context-creation time, so the only way
    // to apply it at runtime is to drop the current context. renderNow() will
    // lazily recreate it with the new swap interval on the next frame; all
    // shader programs / VBOs live in that context and are rebuilt by the
    // subclass's initialize().
    if (m_context)
    {
        // Let the subclass release its GL resources (programs, VAOs, VBOs) and
        // CPU-side layer data while the context is still current, so the Qt
        // GL object destructors can clean up properly.
        resetGpuResources();

        // The batched text device's FBO belongs to the context being dropped;
        // drop it so it gets recreated on the next frame.
        if (m_batchPainter)
        {
            m_batchPainter->end();
        }
        delete m_batchPainter;
        m_batchPainter = nullptr;
        delete m_batchDevice;
        m_batchDevice = nullptr;

        m_context->doneCurrent();
        delete m_context;
        m_context = nullptr;
    }

    renderLater();
}

//! [6]
void OpenglWindow::renderText(int posX, int posY, const QString& text)
{
    if (text.isEmpty())
    {
        return;
    }

    bool shared = false;
    QPainter* painter = activeTextPainter(shared);
    if (!painter)
    {
        return;
    }

    painter->save();
    painter->setPen(QColor(255, 255, 255, 127));
    QFont font(QStringLiteral("Courier New"));
    font.setPixelSize(16 * devicePixelRatio());
    font.setBold(true);
    painter->setFont(font);
    painter->drawText(posX, posY, text);
    painter->restore();
    if (!shared)
    {
        painter->end();
    }
}
//! [6]

//! [7]
void OpenglWindow::calculateFPS()
{
    m_fps = m_fpsCounter;
    m_fpsCounter = 0;
    emit signal_setFps(m_fps);
}
//! [7]
//! [8]
void OpenglWindow::renderShape(const QRect& rec)
{
    bool shared = false;
    QPainter* painter = activeTextPainter(shared);
    if (painter)
    {
        painter->setPen(QColor(0, 0, 0));
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(QBrush(QColor(0, 255, 0, 63), Qt::SolidPattern));
        painter->drawRect(rec);
        if (!shared)
        {
            painter->end();
        }
    }
}

void OpenglWindow::drawLines(const QVector<QPointF>& pointPairs)
{
    if (pointPairs.isEmpty())
    {
        return;
    }

    bool shared = false;
    QPainter* painter = activeTextPainter(shared);
    if (painter)
    {
        painter->setPen(QPen(QColor(255, 255, 255, 127), 1 * devicePixelRatio()));
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawLines(pointPairs);
        if (!shared)
        {
            painter->end();
        }
    }
}
//! [8]
//! [9]
void OpenglWindow::resetGeometry(QRect a_rect)
{
    setGeometry(a_rect);
}
//! [9]

//! [10]
void OpenglWindow::setMapOpMask(MapOpMaskBits a_layer /*, bool a_b*/)
{
    /*
    if(a_b)
        m_uiMapOpMask |= a_layer;
    else
        m_uiMapOpMask &= ~a_layer;
        */
    m_uiMapOpMask = a_layer;
}
//! [10]

//! [11]
void OpenglWindow::renderText(int posX, int posY, const QString& text, const QString& font)
{
    if (text.isEmpty())
    {
        return;
    }

    bool shared = false;
    QPainter* painter = activeTextPainter(shared);
    if (!painter)
    {
        return;
    }

    painter->save();
    painter->setPen(QColor(255, 255, 127, 160));
    // Reuse a cached QFont + QFontMetricsF instead of rebuilding them per
    // label (family-string parse + metrics computation is the dominant cost
    // across tens of thousands of labels per frame).
    const int px = 12 * devicePixelRatio();
    FontEntry* fe = getCachedFont(font, px, false);
    painter->setFont(fe->font);
    const qreal adv = fe->metrics.horizontalAdvance(text);
    painter->drawText(posX - adv / 2, posY + 12 * devicePixelRatio(), text);
    painter->restore();
    if (!shared)
    {
        painter->end();
    }
}
//! [11]

//! [12]
void OpenglWindow::renderText(int posX, int posY, const QString& text, const QString& a_font, qreal a_angle)
{
    if (text.isEmpty())
    {
        return;
    }

    bool shared = false;
    QPainter* painter = activeTextPainter(shared);
    if (!painter)
    {
        return;
    }

    // Draw directly on the shared paint device (the original approach) rather
    // than baking into a QImage + drawImage, which distorts text on
    // QOpenGLPaintDevice.
    painter->save();
    painter->setPen(QColor(155, 205, 155, 150));

    // Reuse cached QFont + QFontMetricsF (this path is called per road/MRT
    // label, i.e. tens of thousands of times per frame at high zoom).
    const int dpr = devicePixelRatio();
    FontEntry* feBig = getCachedFont(a_font, 18 * dpr, true);
    FontEntry* feText = getCachedFont(a_font, 13 * dpr, true);
    painter->setFont(feBig->font);
    const QFontMetricsF& metrics = feBig->metrics;

    painter->translate(posX, posY);
    painter->rotate(a_angle);
    painter->drawText(0, 14, ">>");

    if (qAbs(a_angle) > 90)
    {
        painter->rotate(180.0);
    }

    painter->setFont(feText->font);
    painter->drawText(8 - metrics.horizontalAdvance(text) / 2, 18, text);
    painter->setPen(QColor(5, 5, 5, 150));
    painter->drawText(7 - metrics.horizontalAdvance(text) / 2, 17, text);
    painter->restore();
    if (!shared)
    {
        painter->end();
    }
}
//! [12]

OpenglWindow::FontEntry* OpenglWindow::getCachedFont(const QString& family, int pixelSize, bool bold)
{
    const QString key = family + QLatin1Char('|') + QString::number(pixelSize) + QLatin1Char('|')
                        + (bold ? QLatin1Char('1') : QLatin1Char('0'));
    QHash<QString, FontEntry>::Iterator it = m_fontCache.find(key);
    if (it != m_fontCache.end())
    {
        return &it.value();
    }

    QFont font(family);
    font.setPixelSize(pixelSize);
    font.setBold(bold);
    FontEntry entry(font, QFontMetricsF(font));
    return &m_fontCache.insert(key, entry).value();
}

QPainter* OpenglWindow::activeTextPainter(bool& shared)
{
    shared = m_inTextFrame && m_batchPainter && m_batchPainter->isActive();
    if (shared)
    {
        return m_batchPainter;
    }

    // Standalone mode: create a painter on demand for callers that are not
    // inside a beginTextFrame()/endTextFrame() block (e.g. drawEBL's label).
    if (!m_batchDevice)
    {
        m_batchDevice = new QOpenGLPaintDevice;
    }
    m_batchDevice->setSize(size() * devicePixelRatio());
    if (!m_batchPainter)
    {
        m_batchPainter = new QPainter;
    }
    m_batchPainter->begin(m_batchDevice);
    return m_batchPainter;
}

void OpenglWindow::selectShader(uint shaderId)
{
    qDebug() << "virtual parent, do nothing";
}

void OpenglWindow::resetGpuResources()
{
    qDebug() << "virtual parent, do nothing";
}
