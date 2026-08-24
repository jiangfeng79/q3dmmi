#include "TSDWindow.h"

#include <QCoreApplication>
#include <QDir>
#include <time.h>   //or #include<ctime> for time function
#include <stdlib.h> //or #include<cstdlib> for srand function.
#include <random>
#include <QPointF>
#include <QVector>
#include <QPolygonF>
#include <cstdlib>
#include <ctime>
#include "hmiios2014.h"
#include "mrt.h"
#include "geoTransform.h"

// screen position to map position
// #define X_SCREEN_COORD_TO_MAP_COORD(X) -m_fMapCenterDeltaX/m_property.scale+((X)-width()/2)/(m_property.scale*m_fScaleFactor)
// #define Y_SCREEN_COORD_TO_MAP_COORD(Y) m_fMapCenterDeltaY/m_property.scale-((Y)-height()/2)/(m_property.scale*m_fScaleFactor)
#define X_SCREEN_COORD_TO_MAP_COORD(X) -m_fMapCenterDeltaX / m_sgCoastal.m_property.scale + ((X) - width() / 2) / (m_sgCoastal.m_property.scale * m_fScaleFactor)
#define Y_SCREEN_COORD_TO_MAP_COORD(Y) m_fMapCenterDeltaY / m_sgCoastal.m_property.scale - ((Y) - height() / 2) / (m_sgCoastal.m_property.scale * m_fScaleFactor)

// Note: the WGS84<->Mercator and metre/build map-space transforms now live in
// geoTransform.h (shared with the layer parsers).

// screen position to lat-long position
#define X_SCREEN_COORD_TO_WGS84(X) m_sgCoastal.m_property.centerX - m_fMapCenterDeltaX / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale) + ((X) - width() * devicePixelRatio() / 2) / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale * m_fScaleFactor)
#define Y_SCREEN_COORD_TO_WGS84(Y) WGS84WEBMERCATOR_TO_WGS84(m_sgCoastal.m_property.centerY + m_fMapCenterDeltaY / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale) - ((Y) - height() * devicePixelRatio() / 2) / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale * m_fScaleFactor))

// lat-long position to screen position, for drawing text
#define X_WGS84_COORD_TO_SCREEN_COORD(X) width() * devicePixelRatio() / 2 + ((X) - m_sgCoastal.m_property.centerX + m_fMapCenterDeltaX / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale)) * m_sgCoastal.m_property.scale * m_fScaleFactor * m_sgCoastal.m_property.mapBuildScale
#define Y_WGS84_COORD_TO_SCREEN_COORD(Y) height() * devicePixelRatio() / 2 - (WGS84_TO_WGS84WEBMERCATOR(Y) - m_sgCoastal.m_property.centerY - m_fMapCenterDeltaY / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale)) * m_sgCoastal.m_property.scale * m_fScaleFactor * m_sgCoastal.m_property.mapBuildScale

#define X_WGS84_COORD_TO_MAP_COORD(X) ((X) - m_sgCoastal.m_property.centerX) * m_sgCoastal.m_property.mapBuildScale
#define Y_WGS84_COORD_TO_MAP_COORD(Y) (WGS84_TO_WGS84WEBMERCATOR(Y) - m_sgCoastal.m_property.centerY) * m_sgCoastal.m_property.mapBuildScale

// scale
#define SCALE (m_sgCoastal.m_property.scale * m_fScaleFactor)

/*
template<int N> void TSDWindow::printMrtStringToScreen()
{
    printMrtStringToScreen<N-1>();
    renderText( X_WGS84_COORD_TO_SCREEN_COORD(mrt[N*2])+7, Y_WGS84_COORD_TO_SCREEN_COORD(mrt[N*2+1])+5, QString(mrt_name[N]), QString("Tahoma"));
}

template<> void TSDWindow::printMrtStringToScreen<0>()
{
    renderText(X_WGS84_COORD_TO_SCREEN_COORD(mrt[0])+7, Y_WGS84_COORD_TO_SCREEN_COORD(mrt[1])+5, QString(mrt_name[0]), QString("Tahoma"));
}

template<int N> void TSDWindow::printStringToScreen(MapLayer & a_layer)
{
    printStringToScreen<N-1>(a_layer);

    int x = X_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[N].coordinate[0]);
    int y =  Y_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[N].coordinate[1]);
    char * lable = (a_layer.m_dbfFileReader.getEntity())[N].stringValue;

    if(x>0&&x<width() &&y>0&&y<height() &&lable!=NULL)
    {
        //qDebug() << "data:" << i << "x:" << (m_sgWaterArea.m_dbfFileReader.getEntity())[i].coordinate[0] <<"y:" <<(m_sgWaterArea.m_dbfFileReader.getEntity())[i].coordinate[1] << "value:" <<(m_sgWaterArea.m_dbfFileReader.getEntity())[i].stringValue;
        renderText(x,y, QString(lable), QString("Tahoma"));
    }
}

template<> void TSDWindow::printStringToScreen<0>(MapLayer & a_layer)
{
    int x = X_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[0].coordinate[0]);
    int y =  Y_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[0].coordinate[1]);
    char * lable = (a_layer.m_dbfFileReader.getEntity())[0].stringValue;

    if(x>0&&x<width() &&y>0&&y<height() &&lable!=NULL)
    {
        //qDebug() << "data:" << i << "x:" << (m_sgWaterArea.m_dbfFileReader.getEntity())[i].coordinate[0] <<"y:" <<(m_sgWaterArea.m_dbfFileReader.getEntity())[i].coordinate[1] << "value:" <<(m_sgWaterArea.m_dbfFileReader.getEntity())[i].stringValue;
        renderText(x,y, QString(lable), QString("Tahoma"));
    }
}
*/

TSDWindow::TSDWindow()
    : m_program(0)
    , m_bgProgram(0)
    , m_lineProgram(0)
    , m_bgMouse(0)
    , m_bgMouseDelta(0)
    , m_bgResolution(0)
    , m_bgTime(0)
    , m_bgShaderId(1)
    , m_sgCoastal("./sgMap/singapore", COASTAL, COASTAL_TEXT)
    , m_sgAmenities("./sgMap/singapore.osm-amenities", AMENITIES, AMENITIES_TEXT)
    , m_sgLandUsages("./sgMap/singapore.osm-landusages", LAND_USAGE, LAND_USAGE_TEXT)
    , m_sgPlaces("./sgMap/singapore.osm-places", PLACES, PLACES_TEXT)
    , m_sgMRT("./sgMap/railways", MRT, MRT_TEXT)
    , m_sgWaterArea("./sgMap/singapore.osm-waterareas", WATER_AREA, WATER_AREA_TEXT)
    , m_sgBuilding("./sgMap/singapore.osm-buildings", BUILDING, BUILDING_TEXT)
    , m_sgMainRoads("./sgMap/singapore.osm-mainroads", MAIN_ROADS, MAIN_ROADS_TEXT)
    , m_sgMotorWays("./sgMap/singapore.osm-motorways", MOTOR_WAYS, MOTOR_WAYS_TEXT)
    , m_sgMinorRoads("./sgMap/singapore.osm-minorroads", MINOR_ROADS, MINOR_ROADS_TEXT)
    , m_sgAirWays("./sgMap/singapore.osm-aeroways", AIR_WAYS, AIR_WAYS_TEXT)
    , m_sgManMade("./sgMap/singapore.osm-polygon", "man_made", MAN_MADE, MAN_MADE_TEXT)
    , m_mrt(nullptr)
    , m_mrtVBO(0)
    , m_eblVBO(0)
    , m_shader(0)
    , m_displayMask(0xff5c032b)
    , m_bAutoZoom(false)
    , m_bAutoSwing(false)
    , m_bShaderToys(false)
    , m_flightThread(nullptr)
    , m_flightWorker(nullptr)
{
    m_sgCoastal.m_bToFill = true;
    m_sgWaterArea.m_bToFill = true;
    m_sgManMade.m_bToFill = true;

    // Inject the parser coupled to each layer's input data. Each layer is now
    // paired with an independent parser that turns its raw input into
    // renderable geometry.
    m_sgCoastal.setParser(new ShapefileLayerParser(m_sgCoastal.m_fileName));
    m_sgAmenities.setParser(new ShapefileLayerParser(m_sgAmenities.m_fileName));
    m_sgPlaces.setParser(new ShapefileLayerParser(m_sgPlaces.m_fileName));
    m_sgLandUsages.setParser(new ShapefileLayerParser(m_sgLandUsages.m_fileName));
    m_sgMRT.setParser(new ShapefileLayerParser(m_sgMRT.m_fileName));
    m_sgWaterArea.setParser(new ShapefileLayerParser(m_sgWaterArea.m_fileName));
    m_sgBuilding.setParser(new ShapefileLayerParser(m_sgBuilding.m_fileName));
    m_sgMainRoads.setParser(new ShapefileLayerParser(m_sgMainRoads.m_fileName));
    m_sgMotorWays.setParser(new ShapefileLayerParser(m_sgMotorWays.m_fileName));
    m_sgMinorRoads.setParser(new ShapefileLayerParser(m_sgMinorRoads.m_fileName));
    m_sgAirWays.setParser(new ShapefileLayerParser(m_sgAirWays.m_fileName));
    m_sgManMade.setParser(new ShapefileLayerParser(m_sgManMade.m_fileName, m_sgManMade.m_layerName));

    setDisplayMask(MAN_MADE_TEXT, false);
    setDisplayMask(MOTOR_WAYS_TEXT, false);
    setDisplayMask(MAIN_ROADS_TEXT, false);
    setDisplayMask(MINOR_ROADS_TEXT, false);
    setDisplayMask(MRT_TEXT, false);

    m_listOfLayers
        << &m_sgCoastal
        << &m_sgWaterArea
        << &m_sgMRT
        << &m_sgAmenities
        << &m_sgLandUsages
        << &m_sgPlaces
        << &m_sgBuilding
        << &m_sgMainRoads
        << &m_sgMotorWays
        << &m_sgMinorRoads
        << &m_sgAirWays
        << &m_sgManMade;

    // Live airflight tracking near Changi. The worker runs on a dedicated
    // thread and polls the adsb.lol API; its tracking table is queued onto the
    // GUI thread and drawn by drawFlightMarkers()/drawFlightLabels().
    m_flightWorker = new TrackerWorker;
    m_flightThread = new QThread(this);
    m_flightWorker->moveToThread(m_flightThread);
    connect(m_flightThread, &QThread::started, m_flightWorker, [this]() {
        // Changi Airport, 30 NM radius.
        m_flightWorker->start(1.3644, 103.9915, 60);
    });
    connect(m_flightWorker, &TrackerWorker::trackingTableUpdated,
            this, &TSDWindow::onTrackingTableUpdated, Qt::QueuedConnection);
    connect(m_flightWorker, &TrackerWorker::fetchFailed, [](const QString& err) {
        qWarning().noquote() << "Flight fetch failed:" << err;
    });
    m_flightThread->start();

    // bool bOK = connect(&(G_P_MAINWINDOW->UIQueue), SIGNAL(signal_send_msg()), this, SLOT(slot_process_msg()));
    // assert(bOK);
}

TSDWindow::~TSDWindow()
{
    // Stop the live flight tracker and let its worker thread finish cleanly.
    // The worker's QTimer lives on the worker thread, so it must be stopped
    // from that thread; calling stop() directly from here (the main thread)
    // triggers "QObject::killTimer: Timers cannot be stopped from another
    // thread". Post stop() into the worker's event loop, then quit and wait.
    if (m_flightWorker)
    {
        QMetaObject::invokeMethod(m_flightWorker, &TrackerWorker::stop, Qt::QueuedConnection);
    }
    if (m_flightThread)
    {
        m_flightThread->quit();
        m_flightThread->wait();
    }
    if (m_flightWorker)
    {
        delete m_flightWorker;
        m_flightWorker = nullptr;
    }
    // m_flightThread is a child of this window and is deleted automatically.
    free(m_mrt);
}

void TSDWindow::MapLayer::buildLayer(MapProperty &a_property, int a_iLayerDepth)
{
    if (!m_parser)
        return;

    LayerParser::Options options;
    options.baseProperty = a_property;
    options.layerDepth = a_iLayerDepth;
    options.isBaseLayer = false;
    options.useWgs84BuildTransform = (m_id == MAN_MADE || m_id == MRT);

    m_geometry = m_parser->parse(options);
    m_property = m_geometry.property;
}

void TSDWindow::MapLayer::buildLayer()
{
    if (!m_parser)
        return;

    LayerParser::Options options;
    options.layerDepth = 0;
    options.isBaseLayer = true;
    options.useWgs84BuildTransform = false;

    m_geometry = m_parser->parse(options);
    m_property = m_geometry.property;
}

void TSDWindow::drawTextWithAngle(TSDWindow::MapLayer &a_layer)
{
    const qreal retinaScale = devicePixelRatio();
    if (a_layer.m_text_id & m_displayMask)
    {
        const std::vector<Label>& labels = a_layer.m_geometry.labels;
        for (size_t i = 0; i < labels.size(); ++i)
        {
            int x = X_WGS84_COORD_TO_SCREEN_COORD(labels[i].longitude) * retinaScale;
            int y = Y_WGS84_COORD_TO_SCREEN_COORD(labels[i].latitude) * retinaScale;
            if (x > 0 && x < width() * retinaScale && y > 0 && y < height() * retinaScale && !labels[i].text.empty())
                renderText(x, y, QString::fromStdString(labels[i].text), QString("Tahoma"), labels[i].angle);
        }
    }
}

void TSDWindow::drawText(TSDWindow::MapLayer &a_layer)
{
    const qreal retinaScale = devicePixelRatio();
    if (a_layer.m_text_id & m_displayMask)
    {
        const std::vector<Label>& labels = a_layer.m_geometry.labels;
        for (size_t i = 0; i < labels.size(); ++i)
        {
            int x = X_WGS84_COORD_TO_SCREEN_COORD(labels[i].longitude) * retinaScale;
            int y = Y_WGS84_COORD_TO_SCREEN_COORD(labels[i].latitude) * retinaScale;
            if (x > 0 && x < width() * retinaScale && y > 0 && y < height() * retinaScale && !labels[i].text.empty())
            {
                renderText(x, y, QString::fromStdString(labels[i].text), QString("Tahoma"));
            }
        }
    }
}

static bool checkGL(const char *stage)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        qWarning() << "GL error at" << stage << ":" << err;
        return false;
    }
    return true;
}

GLuint TSDWindow::loadShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);
    return shader;
}

void TSDWindow::initialize()
{
    m_program = new QOpenGLShaderProgram(this);

    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, QString(":/hmiios2014/vshader.glsl")))
        qWarning() << "Vertex shader compile error:" << m_program->log();
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(":/hmiios2014/fshader.glsl")))
        qWarning() << "Fragment shader compile error:" << m_program->log();

    if (!m_program->link())
    {
        qWarning() << "Shader program link error:" << m_program->log();
        return;
    }

    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");
    m_colorId = m_program->uniformLocation("color_id");
    
    // Background (ShaderToy) shader: separate program, fullscreen triangle.
    m_bgProgram = new QOpenGLShaderProgram(this);
    if (!m_bgProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, QString(":/hmiios2014/vshader.glsl")))
        qWarning() << "Background vertex shader compile error:" << m_bgProgram->log();
    if (!m_bgProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(":/hmiios2014/bgfshader.glsl")))
        qWarning() << "Background fragment shader compile error:" << m_bgProgram->log();
    if (!m_bgProgram->link())
        qWarning() << "Background shader program link error:" << m_bgProgram->log();

    m_bgMatrixUniform = m_bgProgram->uniformLocation("matrix");
    m_bgMouse = m_bgProgram->uniformLocation("mouse");
    m_bgMouseDelta = m_bgProgram->uniformLocation("mouseDelta");
    m_bgResolution = m_bgProgram->uniformLocation("resolution");
    m_bgTime = m_bgProgram->uniformLocation("time");
    m_bgShaderId = m_bgProgram->uniformLocation("shader_id");

    // Line shader: same three stages as m_program (vshader + gshader +
    // fshader). The geometry shader expands each line segment into a filled
    // quad so the line (SHPT_ARC) rings can be thickened beyond the driver's
    // 1px glLineWidth limit. Only the uniforms the line pass actually uses are
    // cached (matrix, color_id, resolution).
    m_lineProgram = new QOpenGLShaderProgram(this);
    if (!m_lineProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, QString(":/hmiios2014/vshader.glsl")))
        qWarning() << "Line vertex shader compile error:" << m_lineProgram->log();
    if (!m_lineProgram->addShaderFromSourceFile(QOpenGLShader::Geometry, QString(":/hmiios2014/gshader.glsl")))
        qWarning() << "Line geometry shader compile error:" << m_lineProgram->log();
    if (!m_lineProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(":/hmiios2014/fshader.glsl")))
        qWarning() << "Line fragment shader compile error:" << m_lineProgram->log();
    if (!m_lineProgram->link())
        qWarning() << "Line shader program link error:" << m_lineProgram->log();

    m_lineMatrixUniform = m_lineProgram->uniformLocation("matrix");
    m_lineColorId = m_lineProgram->uniformLocation("color_id");
    m_lineResolution = m_lineProgram->uniformLocation("resolution");
    m_lineTime = m_lineProgram->uniformLocation("time");

    m_program->bind();
    this->initializeOpenGLFunctions();
    this->glGenVertexArrays(1, &m_vao);
    this->glBindVertexArray(m_vao);
    glGenBuffers(1, &m_mrtVBO);
    glGenBuffers(1, &m_eblVBO);

    for (int i = 0; i < m_listOfLayers.size(); ++i)
    {
        TSDWindow::MapLayer *l_layer = m_listOfLayers[i];
        if (i == 0)
            l_layer->buildLayer(); // sg coastal; base layer
        else
            l_layer->buildLayer(m_sgCoastal.m_property, 0);

        if (!l_layer->m_geometry.vertices.empty())
        {
            glGenBuffers(2, l_layer->m_VBO_ID);
            glBindBuffer(GL_ARRAY_BUFFER, l_layer->m_VBO_ID[0]);
            glBufferData(GL_ARRAY_BUFFER,
                         l_layer->m_geometry.vertices.size() * 3 * sizeof(float),
                         l_layer->m_geometry.vertices.data(),
                         GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    m_mrt = (GLfloat *)malloc(sizeof(mrt));
    for (int i = 0; i < sizeof(mrt) / sizeof(GLfloat) / 2; ++i)
    {
        m_mrt[i * 2] = X_WGS84_COORD_TO_MAP_COORD(mrt[i * 2]);
        m_mrt[i * 2 + 1] = Y_WGS84_COORD_TO_MAP_COORD(mrt[i * 2 + 1]);
    }

    if (m_mrt && m_mrtVBO)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_mrtVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mrt), m_mrt, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

//! [4]

// Draw a single polygon ring as a triangle fan. Assumes the VBO is bound and
// the vertex attribute pointer is set up by the caller's surrounding state.
void TSDWindow::drawPolygonRing(MapLayer &a_layer, int i)
{
    const int vertCount = a_layer.m_geometry.rings[i + 1] - a_layer.m_geometry.rings[i];
    if (vertCount < 3)
        return; // a triangle fan needs at least 3 vertices
    glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
    glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, (void *)(intptr_t)(a_layer.m_geometry.rings[i] * 3 * 4));
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_TRIANGLE_FAN, 0, vertCount);
    glDisableVertexAttribArray(m_posAttr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// PASS 1: write every polygon ring into the stencil buffer (color writes off).
// GL_INVERT toggles the bit, so overlapping rings cancel out (even-odd rule).
void TSDWindow::drawRingsToStencil(MapLayer &a_layer)
{
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // disable writing to color buffer
    glStencilFunc(GL_ALWAYS, 0x1, 0x1);
    glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);

    for (int i = 0; i < (int)a_layer.m_geometry.rings.size() - 1; ++i)
    {
        if (a_layer.m_geometry.renderType[i] == SHPT_POLYGON)
            drawPolygonRing(a_layer, i);
    }
}

// PASS 2: color the pixels whose stencil bit ended up set (odd coverage).
void TSDWindow::drawRingsToColor(MapLayer &a_layer)
{
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // enable writing to color buffer
    glStencilFunc(GL_EQUAL, 0x1, 0x1);               // test if it is odd(1)
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    for (int i = 0; i < (int)a_layer.m_geometry.rings.size() - 1; ++i)
    {
        if (a_layer.m_geometry.renderType[i] == SHPT_POLYGON)
            drawPolygonRing(a_layer, i);
    }
}

// Fill a single ring: stencil pass then color pass, resolved immediately.
void TSDWindow::drawRingFilled(MapLayer &a_layer, int i)
{
    if (a_layer.m_geometry.renderType[i] != SHPT_POLYGON)
        return;

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // disable writing to color buffer
    glStencilFunc(GL_ALWAYS, 0x1, 0x1);
    glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);
    drawPolygonRing(a_layer, i);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // enable writing to color buffer
    glStencilFunc(GL_EQUAL, 0x1, 0x1);               // test if it is odd(1)
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    drawPolygonRing(a_layer, i);
}

void TSDWindow::drawLayerAndFill(MapLayer &a_layer)
{
    // polygon
    glClear(GL_STENCIL_BUFFER_BIT);
    glClearStencil(0x0);

    // enable stencil test
    glEnable(GL_STENCIL_TEST);

    if (a_layer.m_id == WATER_AREA)
    {
        // Batched two-pass: all rings write to the stencil buffer first, then
        // all are colored. This lets the even-odd GL_INVERT cancellation
        // accumulate across the whole layer, so overlapping water polygons and
        // holes resolve correctly.
        drawRingsToStencil(a_layer);
        drawRingsToColor(a_layer);
    }
    else
    {
        // Per-ring: each ring resolves its own stencil->color immediately, so
        // there is no cross-ring cancellation.
        for (int i = 0; i < (int)a_layer.m_geometry.rings.size() - 1; ++i)
            drawRingFilled(a_layer, i);
    }

    glDisable(GL_STENCIL_TEST);
}

void TSDWindow::drawLayer(MapLayer &a_layer)
{
    int totalVerts = a_layer.m_property.totalNumberOfVertex;
    glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
    glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(m_posAttr);

    glPointSize(6);
    for (int i = 0; i < (int)a_layer.m_geometry.rings.size() - 1; ++i)
    {
        int startVert = a_layer.m_geometry.rings[i];
        int vertCount = a_layer.m_geometry.rings[i + 1] - a_layer.m_geometry.rings[i];

        // Skip invalid geometry entries (offset or count out of bounds)
        if (startVert < 0 || vertCount <= 0 || startVert + vertCount > totalVerts)
            continue;

        // polygons
        if (a_layer.m_geometry.renderType[i] == SHPT_POLYGON
            /*|| a_layer.m_geometry.renderType[i] == SHPT_POLYGONZ
            || a_layer.m_geometry.renderType[i] == SHPT_POLYGONM*/
        )
        {
            glDrawArrays(GL_LINE_STRIP, startVert, vertCount);
        }
        // lines (SHPT_ARC) are drawn by the dedicated line shader in the line
        // pass (drawLayerLines), not here.
        // points
        else if (a_layer.m_geometry.renderType[i] == SHPT_POINT)
        {
            glDrawArrays(GL_POINTS, startVert, vertCount);
        }
    }

    glDisableVertexAttribArray(m_posAttr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw only the line (SHPT_ARC) rings of a layer using the dedicated line
// shader (m_lineProgram). The caller must have bound m_lineProgram and set its
// color_id uniform. The geometry shader expands each segment into a filled
// quad, so lines are thickened beyond the driver's 1px glLineWidth limit.
void TSDWindow::drawLayerLines(MapLayer &a_layer)
{
    int totalVerts = a_layer.m_property.totalNumberOfVertex;
    glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
    glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(m_posAttr);
    for (int i = 0; i < (int)a_layer.m_geometry.rings.size() - 1; ++i)
    {
        if (a_layer.m_geometry.renderType[i] != SHPT_ARC)
            continue;

        int startVert = a_layer.m_geometry.rings[i];
        int vertCount = a_layer.m_geometry.rings[i + 1] - a_layer.m_geometry.rings[i];

        // Skip invalid geometry entries (offset or count out of bounds)
        if (startVert < 0 || vertCount <= 0 || startVert + vertCount > totalVerts)
            continue;

        glDrawArrays(GL_LINE_STRIP, startVert, vertCount);
    }

    glDisableVertexAttribArray(m_posAttr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//! [5]
void TSDWindow::render()
{
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    int w = width(), h = height();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
    glEnable(GL_BLEND);
    // glEnable( GL_DEPTH_TEST);
    // glEnable(GL_POINT_SMOOTH);
    // glEnable( GL_POLYGON_SMOOTH);
    // glEnable( GL_LINE_SMOOTH );
    // glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE);

    GLfloat time = (GLfloat)clock() / (GLfloat)CLOCKS_PER_SEC;
    GLfloat mouse[] = {(GLfloat)m_iMousePosX, (GLfloat)m_iMousePosY};
    GLfloat mouseDelta[] = {(GLfloat)m_fMapCenterDeltaX * m_fScaleFactor, (GLfloat)m_fMapCenterDeltaY * m_fScaleFactor};
    GLfloat resolution[] = {(GLfloat)w, (GLfloat)h};

    QMatrix4x4 matrix;
    matrix.ortho(-w / 2, w / 2, -h / 2, h / 2, -500.0, 500.0);

    // matrix.rotate(-m_dRotationAngle/M_PI*180.0, 0, 0, 1);

    matrix.translate((m_fMapCenterDeltaX * m_fScaleFactor), -(m_fMapCenterDeltaY * m_fScaleFactor), 0);

    // l_fScaleFactor =  (abs(sin(time*0.1)));
    if (m_bAutoZoom)
        matrix.scale((abs(sin(time * 0.1))) * m_sgCoastal.m_property.scale * m_fScaleFactor);
    else
        matrix.scale(m_sgCoastal.m_property.scale * m_fScaleFactor);
    // matrix.scale( (abs(sin(time*0.1)))*m_sgCoastal.m_property.scale*m_fScaleFactor);

    // center on the new center postion
    // matrix.rotate(1000.0f * time / screen()->refreshRate(), 0, 0, 1);
    if (m_bAutoSwing)
        matrix.rotate(sin(time) * 10, 0, 0, 1);

    QVector<int> shaderRenderLayers = {0, 11}; // COASTAL and MAN_MADE layers are drawn by shaders
    // Precompute which layers are drawn by the shader pass so the main loop
    // below can skip them with an O(1) lookup instead of scanning the list.
    QVector<bool> shaderRendered(m_listOfLayers.size(), false);
    for (int idx : shaderRenderLayers)
        shaderRendered[idx] = true;

    // PASS 0: m_listOfLayers[0], COASTAL shader, drawn before rest of the layers.
    if (m_bShaderToys && m_bgProgram && m_bgProgram->isLinked())
    {
        m_bgProgram->bind();
        m_bgProgram->setUniformValue(m_bgMatrixUniform, matrix);
        m_bgProgram->setUniformValue(m_bgMouse, mouse[0] * retinaScale, -mouse[1] * retinaScale + resolution[1] / 2);
        m_bgProgram->setUniformValue(m_bgMouseDelta, mouseDelta[0] * retinaScale, -mouseDelta[1] * retinaScale);
        m_bgProgram->setUniformValue(m_bgResolution, resolution[0], resolution[1]);
        m_bgProgram->setUniformValue(m_bgTime, time);
        m_bgProgram->setUniformValue(m_bgShaderId, m_shader);
        this->glBindVertexArray(m_vao);

        foreach(int layerIndex, shaderRenderLayers)
        {
            auto *l_layer = m_listOfLayers[layerIndex];
            if (m_displayMask & l_layer->m_id)
            {
               drawLayerAndFill(*l_layer);
            }
        }

        m_bgProgram->release();
    }

    m_program->bind();
    checkGL("after program bind");
    m_program->setUniformValue(m_matrixUniform, matrix);
    checkGL("after uniform set");

    // Draw layers directly, without legacy display lists.
    this->glBindVertexArray(m_vao);

    for (int i = 0; i < m_listOfLayers.size(); ++i)
    {
        TSDWindow::MapLayer *l_layer = m_listOfLayers[i];

        // When ShaderToys is on, the layers in shaderRenderLayers are already
        // drawn with the ShaderToy background shader in the pass above, so skip
        // them here to avoid overdrawn solid color hiding the pattern.
        if ((m_bShaderToys ? !shaderRendered[i] : true) && (m_displayMask & l_layer->m_id))
        {
            m_program->setUniformValue(m_colorId, myLog2(l_layer->m_id));
            if (l_layer->m_bToFill)
                drawLayerAndFill(*l_layer);
            else
                drawLayer(*l_layer);
        }
    }

    // LINE PASS: draw all line (SHPT_ARC) rings with the dedicated line shader
    // (m_lineProgram). The geometry shader expands each segment into a filled
    // quad so lines are thickened beyond the driver's 1px glLineWidth limit.
    // Drawn after the main polygon/point pass so roads and MRT lines sit on top
    // of the polygon fills.
    if (m_lineProgram && m_lineProgram->isLinked())
    {
        m_lineProgram->bind();
        m_lineProgram->setUniformValue(m_lineMatrixUniform, matrix);
        m_lineProgram->setUniformValue(m_lineResolution, resolution[0], resolution[1]);
        m_lineProgram->setUniformValue(m_lineTime, time);
        this->glBindVertexArray(m_vao);

        for (int i = 0; i < m_listOfLayers.size(); ++i)
        {
            TSDWindow::MapLayer *l_layer = m_listOfLayers[i];
            if ((m_bShaderToys ? !shaderRendered[i] : true) && (m_displayMask & l_layer->m_id))
            {
                m_lineProgram->setUniformValue(m_lineColorId, myLog2(l_layer->m_id));
                drawLayerLines(*l_layer);
            }
        }

        m_lineProgram->release();
    }

    m_program->bind();
    // checkGL("before drawMRTStation");
    drawMRTStation();
    // checkGL("after drawMRTStation");

    drawEBL(X_SCREEN_COORD_TO_MAP_COORD(m_iMouseInitX), Y_SCREEN_COORD_TO_MAP_COORD(m_iMouseInitY), sqrt(m_iMouseDeltaX * m_iMouseDeltaX + m_iMouseDeltaY * m_iMouseDeltaY) / SCALE);
    // checkGL("after drawEBL");

    m_program->release();

    // get mouse pos coord in map
    float mapX = X_SCREEN_COORD_TO_WGS84(m_iMousePosX);
    float mapY = Y_SCREEN_COORD_TO_WGS84(m_iMousePosY);

    // QString X = QString("%1'%2").arg((int)mapX).arg((mapX-(int)mapX)*60.0,6,'f', 3, QChar('0'));
    // QString Y = QString("%1'%2").arg((int)mapY).arg((mapY-(int)mapY)*60.0,6,'f', 3, QChar('0'));
    QString X = QString("%1").arg((mapX), 6, 'f', 6, QChar('0'));
    QString Y = QString("%1").arg((mapY), 6, 'f', 6, QChar('0'));
    QString qScale = QString("%1").arg((SCALE), 6, 'f', 6, QChar('0'));

    // QString X1 = QString("%1'%2").arg((int)mapX).arg((mapX-(int)mapX));
    // QString Y1 = QString("%1'%2").arg((int)mapY).arg((mapY-(int)mapY));

    // Begin a single batched 2D text pass: the underlying QOpenGLPaintDevice/
    // QPainter is created once and shared by every HUD/label draw below
    //beginTextFrame();
    m_inTextFrame = true;

    renderShape(QRect(0, 0, 300 * retinaScale, 80 * retinaScale));
    renderText(10, 18 * retinaScale, QString("Coord: [%1,%2]").arg(X).arg(Y));
    renderText(10, 36 * retinaScale, QString("Scale: [%1]").arg(qScale));
    renderText(10, 54 * retinaScale, QString("1 NM:  "));
    QVector<QPointF> scaleLine;
    scaleLine.append(QPointF(84 * retinaScale, 54 * retinaScale));
    scaleLine.append(QPointF(84 * retinaScale + 1852 * SCALE * retinaScale, 54 * retinaScale));
    drawLines(scaleLine);

    renderText(10, 72 * retinaScale, QString("Refresh Rate: %1").arg(m_fps, 4, 10, QChar(' ')));

    if (SCALE > 0.1)
    {

        for (int i = 0; i < sizeof(mrt) / sizeof(GLfloat) / 2; ++i)
        {
            int x = X_WGS84_COORD_TO_SCREEN_COORD(mrt[i * 2]) * retinaScale;
            int y = Y_WGS84_COORD_TO_SCREEN_COORD(mrt[i * 2 + 1]) * retinaScale;
            if (x > 0 && x < width() * retinaScale && y > 0 && y < height() * retinaScale)
                renderText(x + 7, y + 5, QString(mrt_name[i]), QString("Tahoma"));
        }
        // printMrtStringToScreen<sizeof(mrt)/sizeof(GLfloat)/2-1>();
    }

    if (SCALE > 0.6)
    {
        drawText(m_sgBuilding);
        // printBuildingStringToScreen<34696-1>();
        // printBuildingStringToScreen<2000-1>();
        drawText(m_sgLandUsages);
        drawTextWithAngle(m_sgMotorWays);
        drawTextWithAngle(m_sgMainRoads);
        drawTextWithAngle(m_sgMinorRoads);
        drawTextWithAngle(m_sgMRT);
    }

    if (SCALE > 0.1)
    {
        drawText(m_sgWaterArea);
        drawText(m_sgAmenities);
        drawText(m_sgPlaces);
        drawText(m_sgManMade);
    }

    // Live airflight trails + position vectors, plane silhouettes and callsign
    // labels near Changi (always drawn, independent of zoom, since the
    // aircraft are live and sparse).
    drawFlightTrails();
    drawFlightMarkers();
    drawFlightLabels();

    //endTextFrame();
    m_inTextFrame = false;

    ++m_fpsCounter;

    // smooth speed
    if (m_fMotionSpeed > 0)
    {
        m_fMapCenterDeltaX += m_fMotionSpeed / 50 * cos(m_fMotionDir);
        m_fMapCenterDeltaY += m_fMotionSpeed / 50 * sin(m_fMotionDir);
        m_fMapPrevCenterDeltaX = m_fMapCenterDeltaX;
        m_fMapPrevCenterDeltaY = m_fMapCenterDeltaY;
        m_fMotionSpeed -= 50 / m_fScaleFactor;
    }
}
//! [5]

//! [6]
void TSDWindow::drawEBL(float x, float y, float r)
{
#define granularity 63
    m_program->setUniformValue(m_colorId, 16);
    if (m_uiMapOpMask == EBL)
    {
        // EBL
        std::vector<GLfloat> l_vertexBuffer((granularity + 1) * 2);
        GLfloat l_vertexBuffer2[4];
        int i = 0;
        for (GLdouble angle = 0; angle <= 2 * 3.1416; angle += 0.1, ++i)
        {
            l_vertexBuffer[i * 2] = (x + cos(angle) * r);
            l_vertexBuffer[i * 2 + 1] = (y - sin(angle) * r);
        }

        l_vertexBuffer[granularity * 2] = l_vertexBuffer[0];
        l_vertexBuffer[granularity * 2 + 1] = l_vertexBuffer[1];

        l_vertexBuffer2[0] = x;
        l_vertexBuffer2[1] = y;
        l_vertexBuffer2[2] = X_SCREEN_COORD_TO_MAP_COORD(m_iMousePosX);
        l_vertexBuffer2[3] = Y_SCREEN_COORD_TO_MAP_COORD(m_iMousePosY);

        // glBindBuffer(GL_ARRAY_BUFFER, m_eblVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_eblVBO);
        glBufferData(GL_ARRAY_BUFFER, l_vertexBuffer.size() * sizeof(GLfloat), l_vertexBuffer.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glEnableVertexAttribArray(m_posAttr);
        glDrawArrays(GL_TRIANGLE_FAN, 0, granularity);
        m_program->setUniformValue(m_colorId, 3);
        glDrawArrays(GL_LINE_STRIP, 0, granularity + 1);
        glDisableVertexAttribArray(m_posAttr);

        float angle = 0;
        if (m_iMousePosX != m_iMouseInitX)
            angle = (m_iMousePosX - m_iMouseInitX) >= 0 ? atan((float)(m_iMousePosY - m_iMouseInitY) / (float)(m_iMousePosX - m_iMouseInitX)) / 3.1416 * 180 + 90 : atan((float)(m_iMousePosY - m_iMouseInitY) / (float)(m_iMousePosX - m_iMouseInitX)) / 3.1416 * 180 + 270;
        if (m_bMouseIsPressing)
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(l_vertexBuffer2), l_vertexBuffer2, GL_DYNAMIC_DRAW);
            m_program->setUniformValue(m_colorId, 3);
            glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
            glEnableVertexAttribArray(m_posAttr);
            glDrawArrays(GL_LINES, 0, 2);
            glDisableVertexAttribArray(m_posAttr);

            renderText(m_iMousePosX * devicePixelRatio() + 15, m_iMousePosY * devicePixelRatio() + 20, QString(tr("Angle: %1, Dist: %2")).arg(angle, 5, 'f', 1, QChar('0')).arg(r, 6, 'f', 1, QChar('0')), QString("Courier"));
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}
//! [6]

void TSDWindow::drawMRTStation()
{
    glPointSize(12);
    glBindBuffer(GL_ARRAY_BUFFER, m_mrtVBO);

    m_program->setUniformValue(m_colorId, 5); // EW
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_POINTS, 0, 29);
    glDisableVertexAttribArray(m_posAttr);

    m_program->setUniformValue(m_colorId, 3); // NS
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)(29 * 2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_POINTS, 0, 25);
    glDisableVertexAttribArray(m_posAttr);

    m_program->setUniformValue(m_colorId, 5); // expo, changi
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)(54 * 2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_POINTS, 0, 3);
    glDisableVertexAttribArray(m_posAttr);

    m_program->setUniformValue(m_colorId, 9); // NE
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)(57 * 2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_POINTS, 0, 16);
    glDisableVertexAttribArray(m_posAttr);

    m_program->setUniformValue(m_colorId, 7); // circle
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)(73 * 2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_POINTS, 0, 31);
    glDisableVertexAttribArray(m_posAttr);

    m_program->setUniformValue(m_colorId, 7); // marina bay
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)(104 * 2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_POINTS, 0, 3);
    glDisableVertexAttribArray(m_posAttr);

    m_program->setUniformValue(m_colorId, 19); // downtown
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)(107 * 2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_POINTS, 0, 35);
    glDisableVertexAttribArray(m_posAttr);

    m_program->setUniformValue(m_colorId, 21); // thomason east coast
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void *)(142 * 2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_POINTS, 0, 31);
    glDisableVertexAttribArray(m_posAttr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Receives the live airflight tracking table from the TrackerWorker (emitted
// from the worker thread, queued to the GUI thread). We copy it into a
// member so render() can read it without any cross-thread access.
void TSDWindow::onTrackingTableUpdated(const QHash<QString, TrackedAircraft>& table)
{
    m_flightTable = table;
}

// Draw the live airflight trails (fading position history) + position vectors
// near Changi. Called in the batched 2D text pass, before the plane markers,
// so the trails sit behind the planes. The trail opacity degrades with age:
// the newest segment (adjacent to the plane) is 50% opaque and older segments
// fade toward fully transparent.
void TSDWindow::drawFlightTrails()
{
    if (!(m_displayMask & FLIGHTS) || m_flightTable.isEmpty())
        return;

    const qreal retinaScale = devicePixelRatio();
    bool shared = false;
    QPainter* painter = activeTextPainter(shared);
    if (!painter)
        return;

    painter->setRenderHint(QPainter::Antialiasing);

    const int kMaxTrailSamples = 90; // bound the trail length
    const int kBaseAlpha = 127;      // newest segment = 50% opaque

    for (auto it = m_flightTable.constBegin(); it != m_flightTable.constEnd(); ++it)
    {
        const TrackedAircraft& t = it.value();
        const QList<PositionSample>& hist = t.history;
        const int n = hist.size();
        if (n < 2)
            continue;

        const int start = qMax(0, n - kMaxTrailSamples);
        const int count = n - start;

        // Project the (bounded) history to screen space.
        QVector<QPointF> pts(count);
        for (int i = 0; i < count; ++i)
        {
            pts[i] = QPointF(
                X_WGS84_COORD_TO_SCREEN_COORD(hist[start + i].lon) * retinaScale,
                Y_WGS84_COORD_TO_SCREEN_COORD(hist[start + i].lat) * retinaScale);
        }

        // Trail: per-segment lines whose opacity degrades with age. The newest
        // segment (adjacent to the plane) is 50% opaque; older segments fade
        // toward fully transparent.
        for (int i = 0; i < count - 1; ++i)
        {
            const qreal recency = qreal(i + 1) / qreal(count - 1);
            const int alpha = int(kBaseAlpha * recency);
            if (alpha <= 0)
                continue;
            QPen pen(QColor(255, 165, 0, alpha), 1.5 * retinaScale, Qt::SolidLine, Qt::RoundCap);
            painter->setPen(pen);
            painter->drawLine(pts[i], pts[i + 1]);
        }

        // Position (velocity) vector: a line from the current position in the
        // direction of travel, length proportional to ground speed.
        const Aircraft& ac = t.latest;
        const QPointF& head = pts[count - 1];
        const PositionSample& p0 = hist[n - 2];
        const PositionSample& p1 = hist[n - 1];
        const double dLat = p1.lat - p0.lat;
        const double dLon = (p1.lon - p0.lon) * qCos(p0.lat * M_PI / 180.0);
        const double len = qSqrt(dLat * dLat + dLon * dLon);
        if (len > 1e-9)
        {
            const double ux = dLon / len;
            const double uy = -dLat / len; // screen y is down
            const qreal vlen = qBound(qreal(10), qreal(ac.groundSpeed) * 0.1, qreal(80)) * retinaScale;
            const QPointF tip(head.x() + ux * vlen, head.y() + uy * vlen);
            QPen pen(QColor(255, 165, 0, 200), 1.5 * retinaScale, Qt::SolidLine, Qt::RoundCap);
            painter->setPen(pen);
            painter->drawLine(head, tip);
        }
    }

    if (!shared)
        painter->end();
}

// Draw the live airflight markers as plane silhouettes (screen space) near
// Changi. Called in the batched 2D text pass so the planes keep a fixed size
// regardless of zoom. Each plane is oriented by the aircraft's heading, which
// is derived from the last two position samples in its history.
void TSDWindow::drawFlightMarkers()
{
    if (!(m_displayMask & FLIGHTS) || m_flightTable.isEmpty())
        return;

    const qreal retinaScale = devicePixelRatio();
    bool shared = false;
    QPainter* painter = activeTextPainter(shared);
    if (!painter)
        return;

    // A top-down airplane silhouette in local coordinates, nose pointing up
    // (-Y). Symmetric about the fuselage (Y) axis.
    static const QPointF kPlaneOutline[] = {
        QPointF(0.00, -1.00),  // nose
        QPointF(0.15, -0.60),
        QPointF(0.15, -0.15),
        QPointF(1.10,  0.20),  // right wing tip
        QPointF(1.10,  0.35),
        QPointF(0.15,  0.30),
        QPointF(0.15,  0.75),
        QPointF(0.55,  0.95),  // right tail tip
        QPointF(0.55,  1.05),
        QPointF(0.00,  1.00),  // stern
        QPointF(-0.55, 1.05),
        QPointF(-0.55, 0.95),
        QPointF(-0.15, 0.75),
        QPointF(-0.15, 0.30),
        QPointF(-1.10, 0.35),  // left wing tip
        QPointF(-1.10, 0.20),
        QPointF(-0.15, -0.15),
        QPointF(-0.15, -0.60),
    };
    const int kPlaneVerts = int(sizeof(kPlaneOutline) / sizeof(kPlaneOutline[0]));
    const qreal kPlaneSize = 8.0 * retinaScale; // half-extent in device pixels

    QPolygonF basePlane;
    basePlane.reserve(kPlaneVerts);
    for (int i = 0; i < kPlaneVerts; ++i)
        basePlane << kPlaneOutline[i] * kPlaneSize;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(QColor(255, 165, 0, 235));
    painter->setPen(QPen(QColor(180, 90, 0, 255), 1.0 * retinaScale));

    for (auto it = m_flightTable.constBegin(); it != m_flightTable.constEnd(); ++it)
    {
        const TrackedAircraft& t = it.value();
        const Aircraft& ac = t.latest;
        int x = X_WGS84_COORD_TO_SCREEN_COORD(ac.lon) * retinaScale;
        int y = Y_WGS84_COORD_TO_SCREEN_COORD(ac.lat) * retinaScale;
        if (x < -50 || x > width() * retinaScale + 50 || y < -50 || y > height() * retinaScale + 50)
            continue;

        // Heading (degrees, clockwise from north) from the last two samples.
        qreal heading = 0.0;
        if (t.history.size() >= 2)
        {
            const PositionSample& p0 = t.history[t.history.size() - 2];
            const PositionSample& p1 = t.history[t.history.size() - 1];
            const double dLat = p1.lat - p0.lat;
            const double dLon = (p1.lon - p0.lon) * qCos(p0.lat * M_PI / 180.0);
            if (qAbs(dLat) > 1e-9 || qAbs(dLon) > 1e-9)
                heading = qRadiansToDegrees(atan2(dLon, dLat));
        }

        painter->save();
        painter->translate(x, y);
        painter->rotate(heading);
        painter->drawPolygon(basePlane);
        painter->restore();
    }

    if (!shared)
        painter->end();
}

// Draw the live airflight callsign labels (2D text pass) near Changi. Called
// in the batched 2D text pass so the labels are drawn on top of the map.
void TSDWindow::drawFlightLabels()
{
    if (!(m_displayMask & FLIGHTS_TEXT) || m_flightTable.isEmpty())
        return;

    const qreal retinaScale = devicePixelRatio();
    bool shared = false;
    QPainter* painter = activeTextPainter(shared);
    if (!painter)
        return;

    painter->setPen(QColor(120, 255, 120, 220));
    const int px = 13 * devicePixelRatio();
    FontEntry* fe = getCachedFont(QStringLiteral("Tahoma"), px, true);
    painter->setFont(fe->font);

    for (auto it = m_flightTable.constBegin(); it != m_flightTable.constEnd(); ++it)
    {
        const Aircraft& ac = it.value().latest;
        int x = X_WGS84_COORD_TO_SCREEN_COORD(ac.lon) * retinaScale;
        int y = Y_WGS84_COORD_TO_SCREEN_COORD(ac.lat) * retinaScale;
        if (x > 0 && x < width() * retinaScale && y > 0 && y < height() * retinaScale)
        {
            // Offset to the right of the plane silhouette so the text is clear.
            painter->drawText(x + 20, y - 10, ac.callsign);
            // altitude (ft) as a secondary line
            if (ac.altBaro > 0)
                painter->drawText(x + 20, y + 12, QString::number(ac.altBaro));
        }
    }
    if (!shared)
        painter->end();
}

void TSDWindow::centerMap()
{
    m_fMapCenterDeltaX = 0;
    m_fMapCenterDeltaY = 0;
    m_fMapPrevCenterDeltaX = 0;
    m_fMapPrevCenterDeltaY = 0;
    m_fScaleFactor = 1;
}

void TSDWindow::setDisplayMask(DisplayMaskBits layer, bool b)
{
    if (b)
        m_displayMask |= layer;
    else
        m_displayMask &= ~layer;
}

void TSDWindow::selectShader(uint shaderId)
{
    m_shader = shaderId;
    qDebug() << "set shader to " << shaderId;
}

void TSDWindow::resetGpuResources()
{
    qDebug() << "resetGpuResources ";

    // Called from OpenglWindow::toggleVsync() while the GL context is still
    // current, right before it is destroyed. Everything below must happen
    // before doneCurrent(): the Qt GL object destructors and the raw GL
    // delete calls need a live context to release their resources.

    // Shader programs (parented to this window; delete explicitly so the GL
    // programs are released now instead of at window destruction).
    delete m_program;
    m_program = nullptr;
    delete m_bgProgram;
    m_bgProgram = nullptr;
    delete m_lineProgram;
    m_lineProgram = nullptr;

    // Raw GL objects.
    if (m_vao)
    {
        this->glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_mrtVBO)
    {
        glDeleteBuffers(1, &m_mrtVBO);
        m_mrtVBO = 0;
    }
    if (m_eblVBO)
    {
        glDeleteBuffers(1, &m_eblVBO);
        m_eblVBO = 0;
    }

    // MRT station data (reallocated by initialize()).
    free(m_mrt);
    m_mrt = nullptr;

    // Per-layer CPU data + VBOs, so initialize() can rebuild everything
    // cleanly (buildLayer() re-parses the input layer from disk).
    for (int i = 0; i < m_listOfLayers.size(); ++i)
    {
        TSDWindow::MapLayer *l_layer = m_listOfLayers[i];
        if (l_layer->m_VBO_ID[0] || l_layer->m_VBO_ID[1])
        {
            glDeleteBuffers(2, l_layer->m_VBO_ID);
            l_layer->m_VBO_ID[0] = l_layer->m_VBO_ID[1] = 0;
        }
        if (l_layer->m_parser)
            l_layer->m_parser->freeMemory();
        l_layer->m_geometry.clear();
    }
}
