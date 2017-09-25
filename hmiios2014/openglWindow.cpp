#include "openglWindow.h"
#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>

#include <time.h>
#include <qDebug>

#define MOTION_TIMEOUT 20
//! [1]
OpenglWindow::OpenglWindow(QWindow *parent)
	: QWindow(parent)
	, m_update_pending(false)
	, m_animating(false)
	, m_context(0)
	, m_device(0)
	, m_fps(0)
	, m_fpsCounter(0)
	, m_iMousePosX(0)
	, m_iMousePosY(0)
	, m_iMouseInitX(0)
	, m_iMouseInitY(0)
	, m_iMouseDeltaX(0)
	, m_iMouseDeltaY(0)
	//	, m_iCenterDeltaX(0)
	//	, m_iCenterDeltaY(0)
	, m_fMapCenterDeltaX(0)
	, m_fMapCenterDeltaY(0)
	, m_fMapPrevCenterDeltaX(0)
	, m_fMapPrevCenterDeltaY(0)
	, m_dPrevRotationAngle(0)
	, m_fMotionSpeed(0)
	, m_fMotionDir(0)
	, m_fMousePressTime(0)
	, m_uiMapOpMask(PAN)
	, m_fScaleFactor(1)
	, m_bMouseIsPressing(false)
	, m_windowMaximized(false)

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
	delete m_device;
}
//! [2]
void OpenglWindow::render(QPainter *painter)
{
	Q_UNUSED(painter);
}

void OpenglWindow::initialize()
{
}

void OpenglWindow::render()
{
	if (!m_device)
		m_device = new QOpenGLPaintDevice;


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	m_device->setSize(size());

	QPainter painter(m_device);
	render(&painter);
}
//! [2]

//! [3]
void OpenglWindow::renderLater()
{
	if (!m_update_pending) {
		m_update_pending = true;
		QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
	}
}

bool OpenglWindow::event(QEvent *event)
{
	switch (event->type()) {
	case QEvent::UpdateRequest:
	{
		m_update_pending = false;
		renderNow();
		return true;
	}
	case QEvent::MouseMove:
	{
		QMouseEvent *l_mouseEvent = static_cast<QMouseEvent*>(event);
		m_iMousePosX = l_mouseEvent->x();
		m_iMousePosY = l_mouseEvent->y();
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
				double b = sqrt((m_iMousePosY-m_fMapCenterDeltaY)*(m_iMousePosY-m_fMapCenterDeltaY) + (m_iMousePosX-m_fMapCenterDeltaX)*(m_iMousePosX-m_fMapCenterDeltaX));
				double c = sqrt((m_iMouseInitY-m_fMapCenterDeltaY)*(m_iMouseInitY-m_fMapCenterDeltaY) + (m_iMouseInitX-m_fMapCenterDeltaX)*(m_iMouseInitX-m_fMapCenterDeltaX));
				//b^2+c^2-a^2)/2bc
				//m_dRotationAngle = m_dPrevRotationAngle+atan2(m_iMouseDeltaY,(m_iMouseInitX-m_fMapCenterDeltaX));
				float y = (float)m_iMouseInitY*((float)m_iMousePosX - m_fMapCenterDeltaX)-m_fMapCenterDeltaY*(float)m_iMouseDeltaX;
				float x = (float)m_iMouseInitX-m_fMapCenterDeltaX;
				qDebug() << "y:" <<y << "x:" << x;
				if(y*x >0)
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
		//return QWindow::event(event);
	}
	case QEvent::MouseButtonPress:
	{
		QMouseEvent *l_mouseEvent = static_cast<QMouseEvent*>(event);
		if (l_mouseEvent->buttons() & Qt::LeftButton /*|| l_mouseEvent->buttons() == Qt::RightButton*/)
		{
			qDebug() << "left click";
			m_iMouseInitX = l_mouseEvent->x();
			m_iMouseInitY = l_mouseEvent->y();
			m_iMouseDeltaX = 0;
			m_iMouseDeltaY = 0;
			m_bMouseIsPressing = true;

			m_fMotionSpeed = 0;
			m_fMousePressTime = (GLfloat)clock() / (GLfloat)CLOCKS_PER_SEC;
		}
		else if (l_mouseEvent->buttons() & Qt::RightButton)
		{
			qDebug() << "right click";
			m_iMouseInitX = l_mouseEvent->x();
			m_iMouseInitY = l_mouseEvent->y();
			m_iMouseDeltaX = 0;
			m_iMouseDeltaY = 0;
			m_bMouseIsPressing = true;
			//m_dRotationAngle = 0;
		}
		return true;
	}
	case QEvent::MouseButtonRelease:
	{
		QMouseEvent *l_mouseEvent = static_cast<QMouseEvent*>(event);
		m_bMouseIsPressing = false;
		float l_fMouseReleaseTime = (GLfloat)clock() / (GLfloat)CLOCKS_PER_SEC;
		//m_iCenterDeltaX += m_iMouseDeltaX;
		//m_iCenterDeltaY += m_iMouseDeltaY;
		if (m_uiMapOpMask == PAN)
		{
			m_fMapPrevCenterDeltaX = m_fMapCenterDeltaX;
			m_fMapPrevCenterDeltaY = m_fMapCenterDeltaY;

			if ((l_fMouseReleaseTime - m_fMousePressTime) < 0.2)
			{
				m_fMotionSpeed = sqrt(m_iMouseDeltaX*m_iMouseDeltaX + m_iMouseDeltaY*m_iMouseDeltaY) / (l_fMouseReleaseTime - m_fMousePressTime) / m_fScaleFactor;
				m_fMotionDir = atan2((float)m_iMouseDeltaY, (float)m_iMouseDeltaX);
			}
		}
		m_iMouseDeltaX = 0;
		m_iMouseDeltaY = 0;
		if (l_mouseEvent->button() == Qt::RightButton)
		{
			m_dPrevRotationAngle = m_dRotationAngle;
			//m_dRotationAngle = 0;
			qDebug() << "accumulate prev angle: " << m_dPrevRotationAngle;
		}
		return true;
	}
	case QEvent::Wheel:
	{
		QWheelEvent *l_wheelEvent = static_cast<QWheelEvent*>(event);
		int numDegrees = l_wheelEvent->delta() / 8;
		int numSteps = numDegrees / 15;
		m_fScaleFactor *= pow(1.2, numSteps);
		if (m_fScaleFactor > 1600.0)
			m_fScaleFactor = 1600.0;

		if (m_fScaleFactor < .0001)
			m_fScaleFactor = .0001;



		return true;
	}
	case QEvent::KeyPress:
	{
		QKeyEvent *l_keyevent = static_cast<QKeyEvent*>(event);
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
		return true;
	}
	default:
		return QWindow::event(event);
	}
}

void OpenglWindow::exposeEvent(QExposeEvent *event)
{
	Q_UNUSED(event);

	if (isExposed())
		renderNow();
}
//! [3]

//! [4]
void OpenglWindow::renderNow()
{
	if (!isExposed())
		return;

	bool needsInitialize = false;

	if (!m_context) {
		m_context = new QOpenGLContext(this);
		m_context->setFormat(requestedFormat());
		m_context->create();
		/*m_funcs = m_context->versionFunctions<QOpenGLFunctions_3_3_Core>();
		if ( !m_funcs ) {
		qWarning( "Could not obtain OpenGL versions object" );
		exit( 1 );
		}
		m_funcs->initializeOpenGLFunctions();	*/
		needsInitialize = true;
	}

	m_context->makeCurrent(this);

	if (needsInitialize) {
		initializeOpenGLFunctions();
		initialize();
	}

	render();

	m_context->swapBuffers(this);

	// not to starve the other window events like window move
	QCoreApplication::processEvents();

	if (m_animating)
		renderLater();
}
//! [4]

//! [5]
void OpenglWindow::setAnimating(bool animating)
{
	m_animating = animating;

	if (animating)
		renderLater();
}
//! [5]

//! [6]
void OpenglWindow::renderText(int posX, int posY, QString & text)
{
	if (!m_device)
		m_device = new QOpenGLPaintDevice;
	m_device->setSize(size());

	if (m_device)
	{
		QPainter painter(m_device);
		//painter.beginNativePainting();
		painter.setPen(QColor(255, 255, 255, 127));
		//QFont font("Sans Serif");
		QFont font("Monospace");
		font.setStyleHint(QFont::TypeWriter);
		font.setPixelSize(16);
		font.setBold(true);
		//painter.setRenderHint(QPainter::Antialiasing);
		painter.setFont(font);
		//painter.setBrush(QBrush(QColor(0, 255, 0, 127), Qt::SolidPattern));
		painter.drawText(posX, posY, text);
		//painter.endNativePainting();
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
void OpenglWindow::renderShape(QRect & rec)
{
	if (!m_device)
		m_device = new QOpenGLPaintDevice;
	//m_device->setSize(size());

	//if(m_device)
	{
		QPainter painter(m_device);

		//painter.beginNativePainting();
		painter.setPen(QColor(0, 0, 0));
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setBrush(QBrush(QColor(0, 255, 0, 63), Qt::SolidPattern));
		painter.drawRect(rec);
		//painter.endNativePainting();
	}
}

void OpenglWindow::drawLines(const QVector<QPointF> & pointPairs)
{
	if (!m_device)
		m_device = new QOpenGLPaintDevice;
	m_device->setSize(size());

	if (m_device)
	{
		QPainter painter(m_device);

		//painter.beginNativePainting();
		painter.setPen(QColor(255, 255, 255, 127));
		painter.setRenderHint(QPainter::Antialiasing);
		//painter.setBrush(QBrush(QColor(0, 255, 0, 63), Qt::SolidPattern));
		painter.drawLines(pointPairs);
		//painter.endNativePainting();
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
void OpenglWindow::setMapOpMask(MapOpMaskBits a_layer/*, bool a_b*/)
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
void OpenglWindow::renderText(int posX, int posY, QString & text, QString &font)
{
	if (!m_device)
		m_device = new QOpenGLPaintDevice;

	if (m_device)
	{
		m_device->setSize(size());

		QPainter painter(m_device);

		//painter.beginNativePainting();
		painter.setPen(QColor(255, 255, 127, 160));
		QFontMetrics metrics(painter.font());
		//QFont font = painter.font();
		//font.setBold(true);


		//QFont font("Sans Serif");
		//QFont font(font);
		//font.setStyleHint(QFont::TypeWriter);
		//font.setPixelSize(11);
		//font.setBold(true);
		//painter.setRenderHint(QPainter::Antialiasing);
		//painter.setFont(font);
		//painter.setBrush(QBrush(QColor(0, 255, 0, 127), Qt::SolidPattern));*/
		painter.drawText(posX - metrics.width(text) / 2, posY + 12, text);

		//painter.endNativePainting();
	}
}
//! [11]

//! [12]
void OpenglWindow::renderText(int posX, int posY, QString & text, QString &a_font, qreal a_angle)
{
	if (!m_device)
		m_device = new QOpenGLPaintDevice;
	m_device->setSize(size());


	if (m_device)
	{
		QPainter painter(m_device);

		//painter.beginNativePainting();
		painter.setPen(QColor(155, 205, 155, 150));
		//painter.setPen(QColor(5, 5, 5, 150));

		//painter.save();
		//QFont font(a_font);
		//font.setStyleHint(QFont::TypeWriter);
		m_font.setPixelSize(18);
		m_font.setBold(true);
		painter.setFont(m_font);
		QFontMetrics metrics(painter.font());
		painter.translate(posX, posY);
		painter.rotate(a_angle);
		painter.drawText(0, 4, ">>");
		//painter.setPen(QColor(5, 5, 5, 150));
		//painter.drawText(-1,3, ">>");

		if (abs(a_angle) > 90)
			painter.rotate(180.0);

		//painter.setPen(QColor(155, 205, 155, 150));
		m_font.setPixelSize(13);
		m_font.setBold(true);
		painter.setFont(m_font);
		painter.drawText(8 - metrics.width(text) / 2, 18, text);
		painter.setPen(QColor(5, 5, 5, 150));
		//painter.setPen(QColor(255, 105, 105, 150));

		painter.drawText(7 - metrics.width(text) / 2, 17, text);
		//painter.restore();
		//painter.endNativePainting();
	}
}
//! [12]

