#include "TSDWindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QPointF>
#include <QPolygonF>
#include <QVector>

#include <cstdlib>
#include <ctime>
#include <random>
#include <stdlib.h>  //or #include<cstdlib> for srand function.
#include <time.h>    //or #include<ctime> for time function

#include "geoTransform.h"
#include "hmiios2014.h"
#include "mrt.h"

// screen position to map position
// #define X_SCREEN_COORD_TO_MAP_COORD(X)
// -m_fMapCenterDeltaX/m_property.scale+((X)-width()/2)/(m_property.scale*m_fScaleFactor) #define
// Y_SCREEN_COORD_TO_MAP_COORD(Y) m_fMapCenterDeltaY/m_property.scale-((Y)-height()/2)/(m_property.scale*m_fScaleFactor)
#define X_SCREEN_COORD_TO_MAP_COORD(X)                 \
    -m_fMapCenterDeltaX / m_sgCoastal.m_property.scale \
        + ((X) - width() / 2) / (m_sgCoastal.m_property.scale * m_fScaleFactor)
#define Y_SCREEN_COORD_TO_MAP_COORD(Y)                \
    m_fMapCenterDeltaY / m_sgCoastal.m_property.scale \
        - ((Y) - height() / 2) / (m_sgCoastal.m_property.scale * m_fScaleFactor)

// Note: the WGS84<->Mercator and metre/build map-space transforms now live in
// geoTransform.h (shared with the layer parsers).

// screen position to lat-long position
#define X_SCREEN_COORD_TO_WGS84(X)                                                                   \
    m_sgCoastal.m_property.centerX                                                                   \
        - m_fMapCenterDeltaX / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale) \
        + ((X) - width() * devicePixelRatio() / 2)                                                   \
              / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale * m_fScaleFactor)
#define Y_SCREEN_COORD_TO_WGS84(Y)                                                                   \
    WGS84WEBMERCATOR_TO_WGS84(                                                                       \
        m_sgCoastal.m_property.centerY                                                               \
        + m_fMapCenterDeltaY / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale) \
        - ((Y) - height() * devicePixelRatio() / 2)                                                  \
              / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale * m_fScaleFactor))

// lat-long position to screen position, for drawing text
#define X_WGS84_COORD_TO_SCREEN_COORD(X)                                                                 \
    width() * devicePixelRatio() / 2                                                                     \
        + ((X) - m_sgCoastal.m_property.centerX                                                          \
           + m_fMapCenterDeltaX / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale)) \
              * m_sgCoastal.m_property.scale * m_fScaleFactor * m_sgCoastal.m_property.mapBuildScale
#define Y_WGS84_COORD_TO_SCREEN_COORD(Y)                                                                 \
    height() * devicePixelRatio() / 2                                                                    \
        - (WGS84_TO_WGS84WEBMERCATOR(Y) - m_sgCoastal.m_property.centerY                                 \
           - m_fMapCenterDeltaY / (m_sgCoastal.m_property.mapBuildScale * m_sgCoastal.m_property.scale)) \
              * m_sgCoastal.m_property.scale * m_fScaleFactor * m_sgCoastal.m_property.mapBuildScale

#define X_WGS84_COORD_TO_MAP_COORD(X) ((X) - m_sgCoastal.m_property.centerX) * m_sgCoastal.m_property.mapBuildScale
#define Y_WGS84_COORD_TO_MAP_COORD(Y) \
    (WGS84_TO_WGS84WEBMERCATOR(Y) - m_sgCoastal.m_property.centerY) * m_sgCoastal.m_property.mapBuildScale

// scale
#define SCALE (m_sgCoastal.m_property.scale * m_fScaleFactor)

TSDWindow::TSDWindow()
    : m_program(0),
      m_bgProgram(0),
      m_lineProgram(0),
      m_bgMouse(0),
      m_bgMouseDelta(0),
      m_bgResolution(0),
      m_bgTime(0),
      m_bgShaderId(1),
      m_displayMask(0x3DFF5C032B),
      m_sgCoastal("./sgMap/singapore", COASTAL, COASTAL_TEXT, new ShapefileLayerParser("./sgMap/singapore"),
                  m_displayMask, *this),
      m_sgAmenities("./sgMap/singapore.osm-amenities", AMENITIES, AMENITIES_TEXT,
                    new ShapefileLayerParser("./sgMap/singapore.osm-amenities"), m_displayMask, *this),
      m_sgLandUsages("./sgMap/singapore.osm-landusages", LAND_USAGE, LAND_USAGE_TEXT,
                     new ShapefileLayerParser("./sgMap/singapore.osm-landusages"), m_displayMask, *this),
      m_sgPlaces("./sgMap/singapore.osm-places", PLACES, PLACES_TEXT,
                 new ShapefileLayerParser("./sgMap/singapore.osm-places"), m_displayMask, *this),
      m_sgMRT("./sgMap/railways", MRT, MRT_TEXT, new ShapefileLayerParser("./sgMap/railways"), m_displayMask, *this),
      m_sgWaterArea("./sgMap/singapore.osm-waterareas", WATER_AREA, WATER_AREA_TEXT,
                    new ShapefileLayerParser("./sgMap/singapore.osm-waterareas"), m_displayMask, *this,
                    MapLayer::FillMode::Substract),
      m_sgBuilding("./sgMap/singapore.osm-buildings", BUILDING, BUILDING_TEXT,
                   new ShapefileLayerParser("./sgMap/singapore.osm-buildings"), m_displayMask, *this),
      m_sgMainRoads("./sgMap/singapore.osm-mainroads", MAIN_ROADS, MAIN_ROADS_TEXT,
                    new ShapefileLayerParser("./sgMap/singapore.osm-mainroads"), m_displayMask, *this),
      m_sgMotorWays("./sgMap/singapore.osm-motorways", MOTOR_WAYS, MOTOR_WAYS_TEXT,
                    new ShapefileLayerParser("./sgMap/singapore.osm-motorways"), m_displayMask, *this),
      m_sgMinorRoads("./sgMap/singapore.osm-minorroads", MINOR_ROADS, MINOR_ROADS_TEXT,
                     new ShapefileLayerParser("./sgMap/singapore.osm-minorroads"), m_displayMask, *this),
      m_sgAirWays("./sgMap/singapore.osm-aeroways", AIR_WAYS, AIR_WAYS_TEXT,
                  new ShapefileLayerParser("./sgMap/singapore.osm-aeroways"), m_displayMask, *this),
      m_sgManMade("./sgMap/singapore.osm-polygon", MAN_MADE, MAN_MADE_TEXT,
                  new ShapefileLayerParser("./sgMap/singapore.osm-polygon", "man_made"), m_displayMask, *this),
      m_sgFlightTrails(FLIGHT_TRAILS, FLIGHTS_TEXT, new FlightLayerParser(FlightLayerParser::Trails), m_displayMask,
                       *this),
      m_sgFlightMarkers(FLIGHTS, FLIGHTS_TEXT, new FlightLayerParser(FlightLayerParser::Markers), m_displayMask, *this,
                        LiveMapLayer::LabelStyle::Flight, MapLayer::FillMode::Fill),
      m_sgBusRouteLines(BUS_ROUTES, BUS_ROUTES_TEXT, new BusLayerParser(BusLayerParser::RouteLines), m_displayMask,
                        *this),
      m_sgBusRouteLines2(BUS_ROUTES2, BUS_ROUTES_TEXT, new BusLayerParser(BusLayerParser::RouteLines), m_displayMask,
                         *this),
      m_sgBusStops(BUS_STOPS, BUS_STOPS_TEXT, new BusLayerParser(BusLayerParser::RouteStops), m_displayMask, *this),
      m_sgBusStops2(BUS_STOPS2, BUS_STOPS_TEXT, new BusLayerParser(BusLayerParser::RouteStops), m_displayMask, *this),
      m_sgBusVehicles(BUS_TRACKS, BUS_TRACKS_TEXT, new BusLayerParser(BusLayerParser::Vehicles), m_displayMask, *this,
                      LiveMapLayer::LabelStyle::Default, MapLayer::FillMode::Fill),
      m_sgBusWindshields(BUS_TRACKS_WINDSHIELD, BUS_STOPS_TEXT, new BusLayerParser(BusLayerParser::VehicleWindshields),
                         m_displayMask, *this, LiveMapLayer::LabelStyle::Default, MapLayer::FillMode::Fill),
      m_mrt(nullptr),
      m_mrtVBO(0),
      m_eblVBO(0),
      m_shader(0),
      m_bAutoZoom(false),
      m_bAutoSwing(false),
      m_bShaderToys(false)
{
    m_programSlots = {&m_program, &m_bgProgram, &m_lineProgram};

    m_baseLayers = {&m_sgCoastal, &m_sgManMade};
    m_staticLayers = {&m_sgMRT,       &m_sgAmenities, &m_sgLandUsages, &m_sgPlaces,  &m_sgBuilding,
                      &m_sgMainRoads, &m_sgMotorWays, &m_sgMinorRoads, &m_sgAirWays, &m_sgWaterArea};
    m_liveLayers = {&m_sgFlightTrails, &m_sgFlightMarkers, &m_sgBusRouteLines, &m_sgBusRouteLines2,
                    &m_sgBusStops,     &m_sgBusStops2,     &m_sgBusVehicles,   &m_sgBusWindshields};

    // Live airflight tracking near Changi. The worker runs on a dedicated
    // thread and polls the adsb.lol API; its tracking table is queued onto the
    // GUI thread, turned into flight MapLayers, and drawn through the normal
    // map-layer passes.
    m_workers.emplace_back(std::make_unique<WorkerEntryImpl<TrackerWorker>>(this));
    m_workers.emplace_back(std::make_unique<WorkerEntryImpl<BusTrackerWorker>>(this));
    m_workers.emplace_back(std::make_unique<WorkerEntryImpl<BusRouteWorker>>(this));

    auto* flightWorker = workerAt<TrackerWorker>(m_workers, FlightWorkerIndex);
    auto* busWorker = workerAt<BusTrackerWorker>(m_workers, BusWorkerIndex);
    auto* busRouteWorker = workerAt<BusRouteWorker>(m_workers, BusRouteWorkerIndex);

    connect(m_workers[FlightWorkerIndex]->thread(), &QThread::started, flightWorker, [flightWorker]() {
        // Changi Airport, 30 NM radius.
        flightWorker->start(1.3644, 103.9915, 60);
    });
    connect(flightWorker, &TrackerWorker::trackingTableUpdated, this, &TSDWindow::onTrackingTableUpdated,
            Qt::QueuedConnection);
    connect(flightWorker, &TrackerWorker::fetchFailed,
            [](const QString& err) { qWarning().noquote() << "Flight fetch failed:" << err; });
    m_workers[FlightWorkerIndex]->thread()->start();

    // Live sg bus arrival time tracker per bus stop. The worker runs on a dedicated
    // thread and polls the LTA DataMall API.
    connect(busWorker, &BusTrackerWorker::busArrivalUpdated, this, &TSDWindow::onBusArrivalUpdated,
            Qt::QueuedConnection);
    connect(busWorker, &BusTrackerWorker::fetchFailed,
            [](const QString& err) { qWarning().noquote() << "Bus fetch failed:" << err; });

    // Bus route worker running on a dedicated worker thread.
    connect(busRouteWorker, &BusRouteWorker::busRouteReady, this, &TSDWindow::onBusRouteReady, Qt::QueuedConnection);
    connect(busRouteWorker, &BusRouteWorker::fetchFailed,
            [](const QString& err) { qWarning().noquote() << "Bus route fetch failed:" << err; });

    // bool bOK = connect(&(G_P_MAINWINDOW->UIQueue), SIGNAL(signal_send_msg()), this, SLOT(slot_process_msg()));
    // assert(bOK);
}

TSDWindow::~TSDWindow()
{
    for (const auto& entry : m_workers)
    {
        entry->stop();
    }
    for (const auto& entry : m_workers)
    {
        if (entry->thread()->isRunning())
        {
            entry->thread()->quit();
            entry->thread()->wait();
        }
    }
    m_workers.clear();
    free(m_mrt);
}

static bool checkGL(const char* stage)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        qWarning() << "GL error at" << stage << ":" << err;
        return false;
    }
    return true;
}

GLuint TSDWindow::loadShader(GLenum type, const char* source)
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
    {
        qWarning() << "Vertex shader compile error:" << m_program->log();
    }
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(":/hmiios2014/fshader.glsl")))
    {
        qWarning() << "Fragment shader compile error:" << m_program->log();
    }

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
    {
        qWarning() << "Background vertex shader compile error:" << m_bgProgram->log();
    }
    if (!m_bgProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(":/hmiios2014/bgfshader.glsl")))
    {
        qWarning() << "Background fragment shader compile error:" << m_bgProgram->log();
    }
    if (!m_bgProgram->link())
    {
        qWarning() << "Background shader program link error:" << m_bgProgram->log();
    }

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
    {
        qWarning() << "Line vertex shader compile error:" << m_lineProgram->log();
    }
    if (!m_lineProgram->addShaderFromSourceFile(QOpenGLShader::Geometry, QString(":/hmiios2014/gshader.glsl")))
    {
        qWarning() << "Line geometry shader compile error:" << m_lineProgram->log();
    }
    if (!m_lineProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(":/hmiios2014/fshader.glsl")))
    {
        qWarning() << "Line fragment shader compile error:" << m_lineProgram->log();
    }
    if (!m_lineProgram->link())
    {
        qWarning() << "Line shader program link error:" << m_lineProgram->log();
    }

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

    for (BaseMapLayer* layer : m_baseLayers)
    {
        layer->buildLayer(m_sgCoastal.m_property, 0);
    }
    for (StaticMapLayer* layer : m_staticLayers)
    {
        layer->buildLayer(m_sgCoastal.m_property, 0);
    }
    m_mrt = (GLfloat*)malloc(sizeof(mrt));
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

//! [5]
void TSDWindow::render()
{
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    int w = width(), h = height();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE);

    rebuildLiveLayers();

    GLfloat time = (GLfloat)clock() / (GLfloat)CLOCKS_PER_SEC;
    GLfloat mouse[] = {(GLfloat)m_iMousePosX, (GLfloat)m_iMousePosY};
    GLfloat mouseDelta[] = {(GLfloat)m_fMapCenterDeltaX * m_fScaleFactor, (GLfloat)m_fMapCenterDeltaY * m_fScaleFactor};
    GLfloat resolution[] = {(GLfloat)w, (GLfloat)h};

    QMatrix4x4 matrix;
    matrix.ortho(-w / 2, w / 2, -h / 2, h / 2, -500.0, 500.0);
    matrix.translate((m_fMapCenterDeltaX * m_fScaleFactor), -(m_fMapCenterDeltaY * m_fScaleFactor), 0);

    if (m_bAutoZoom)
    {
        matrix.scale((abs(sin(time * 0.1))) * m_sgCoastal.m_property.scale * m_fScaleFactor);
    }
    else
    {
        matrix.scale(m_sgCoastal.m_property.scale * m_fScaleFactor);
    }

    if (m_bAutoSwing)
    {
        matrix.rotate(sin(time) * 10, 0, 0, 1);
    }

    const MapLayerRenderContext layerContext = {
        m_posAttr,
        retinaScale,
        w,
        h,
        [this, retinaScale](double longitude, double latitude) {
            return QPointF(X_WGS84_COORD_TO_SCREEN_COORD(longitude) * retinaScale,
                           Y_WGS84_COORD_TO_SCREEN_COORD(latitude) * retinaScale);
        },
        [this](int x, int y, const QString& text) { renderText(x, y, text, QStringLiteral("Tahoma")); },
        [this](int x, int y, const QString& text, float angle) {
            renderText(x, y, text, QStringLiteral("Tahoma"), angle);
        },
        [this, retinaScale](const QString& text) {
            return getCachedFont(QStringLiteral("Tahoma"), 12 * retinaScale, false)->metrics.horizontalAdvance(text);
        }};

    // PASS 0: coastal and man-made shader layers, drawn before all other layers.
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

        for (MapLayer* layer : m_baseLayers)
        {
            layer->draw(layerContext);
        }

        m_bgProgram->release();
    }

    m_program->bind();
    m_program->setUniformValue(m_matrixUniform, matrix);

    // Draw layers directly, without legacy display lists.
    this->glBindVertexArray(m_vao);

    const auto drawLayers = [this, &layerContext](const auto& layers) {
        for (MapLayer* layer : layers)
        {
            m_program->setUniformValue(m_colorId, myLog2(layer->id()));
            layer->draw(layerContext);
        }
    };
    const auto drawBaseLayers = [this, &layerContext](const auto& layers) {
        for (MapLayer* layer : layers)
        {
            if (!m_bShaderToys)
            {
                m_program->setUniformValue(m_colorId, myLog2(layer->id()));
                layer->draw(layerContext);
            }
        }
    };

    drawBaseLayers(m_baseLayers);
    drawLayers(m_staticLayers);
    drawLayers(m_liveLayers);

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

        const auto drawLines = [this, &layerContext](const auto& layers) {
            for (MapLayer* layer : layers)
            {
                m_lineProgram->setUniformValue(m_lineColorId, myLog2(layer->id()));
                layer->draw(layerContext, true);
            }
        };
        drawLines(m_baseLayers);
        drawLines(m_staticLayers);
        drawLines(m_liveLayers);

        m_lineProgram->release();
    }

    m_program->bind();

    // checkGL("before drawMRTStation");
    drawMRTStation();
    // checkGL("after drawMRTStation");

    drawEBL(X_SCREEN_COORD_TO_MAP_COORD(m_iMouseInitX), Y_SCREEN_COORD_TO_MAP_COORD(m_iMouseInitY),
            sqrt(m_iMouseDeltaX * m_iMouseDeltaX + m_iMouseDeltaY * m_iMouseDeltaY) / SCALE);
    // checkGL("after drawEBL");

    m_program->release();

    // get mouse pos coord in map
    float mapX = X_SCREEN_COORD_TO_WGS84(m_iMousePosX);
    float mapY = Y_SCREEN_COORD_TO_WGS84(m_iMousePosY);

    QString X = QString("%1").arg((mapX), 6, 'f', 6, QChar('0'));
    QString Y = QString("%1").arg((mapY), 6, 'f', 6, QChar('0'));
    QString qScale = QString("%1").arg((SCALE), 6, 'f', 6, QChar('0'));

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

    if ((m_displayMask & MRT_POINT_TEXT) && SCALE > 0.5)
    {
        for (int i = 0; i < sizeof(mrt) / sizeof(GLfloat) / 2; ++i)
        {
            int x = X_WGS84_COORD_TO_SCREEN_COORD(mrt[i * 2]) * retinaScale;
            int y = Y_WGS84_COORD_TO_SCREEN_COORD(mrt[i * 2 + 1]) * retinaScale;
            if (x > 0 && x < width() * retinaScale && y > 0 && y < height() * retinaScale)
            {
                renderText(x + 7, y + 5, QString(mrt_name[i]), QString("Tahoma"));
            }
        }
    }

    if ((m_displayMask & BUS_ROUTES) && !m_activeBusRoutes.isEmpty())
    {
        int pointCount = 0;
        for (const BusRoute& route : m_activeBusRoutes)
        {
            for (const RouteStop& rstop : route.stops)
            {
                if (rstop.stop.latitude != 0.0 || rstop.stop.longitude != 0.0)
                {
                    ++pointCount;
                }
            }
        }

        renderText(10, 100 * retinaScale, QString("Bus Route: %1 (%2 stops)").arg(m_currentBusNo).arg(pointCount));

        if ((m_displayMask & BUS_ROUTES_TEXT) && SCALE > 0.5)
        {
            for (const BusRoute& route : m_activeBusRoutes)
            {
                for (const RouteStop& rstop : route.stops)
                {
                    if (rstop.stop.latitude == 0.0 && rstop.stop.longitude == 0.0)
                    {
                        continue;
                    }
                    int x = X_WGS84_COORD_TO_SCREEN_COORD(rstop.stop.longitude) * retinaScale;
                    int y = Y_WGS84_COORD_TO_SCREEN_COORD(rstop.stop.latitude) * retinaScale;
                    if (x > -50 && x < width() * retinaScale + 50 && y > -50 && y < height() * retinaScale + 50)
                    {
                        QString desc =
                            rstop.stop.description.isEmpty() ? rstop.stop.busStopCode : rstop.stop.description;
                        QString label = QString("D%1: %2 (%3)").arg(route.direction).arg(desc, rstop.stop.busStopCode);
                        renderText(x + 7, y + 5, label, QString("Tahoma"));
                    }
                }
            }
        }
    }

    if ((m_displayMask & BUS_TRACKS) && !m_currentBusStopSnapshot.busStopCode.isEmpty())
    {
        renderText(10, 120 * retinaScale,
                   QString("Bus Track Stop: %1 (%2 services, Updated %3)")
                       .arg(m_currentBusStopSnapshot.busStopCode)
                       .arg(m_currentBusStopSnapshot.services.size())
                       .arg(m_currentBusStopSnapshot.lastUpdated.toString("hh:mm:ss")));

        if ((m_displayMask & BUS_TRACKS_TEXT) && SCALE > 0.05)
        {
            auto getMinsToArr = [](const QString& timeStr) -> int {
                if (timeStr.isEmpty())
                    return -1;
                QDateTime dt = QDateTime::fromString(timeStr, Qt::ISODate);
                if (!dt.isValid())
                    return -1;
                qint64 secs = QDateTime::currentDateTime().secsTo(dt);
                return std::max(0, static_cast<int>(secs / 60));
            };

            const auto busInfos = m_sgBusVehicles.m_parser
                                      ? static_cast<BusLayerParser*>(m_sgBusVehicles.m_parser)->getBusInfos()
                                      : std::vector<BusLayerParser::TrackedBusInfo>();

            for (const auto& info : busInfos)
            {
                int x = X_WGS84_COORD_TO_SCREEN_COORD(info.bus.longitude) * retinaScale;
                int y = Y_WGS84_COORD_TO_SCREEN_COORD(info.bus.latitude) * retinaScale;
                if (x > -50 && x < width() * retinaScale + 50 && y > -50 && y < height() * retinaScale + 50)
                {
                    int mins = getMinsToArr(info.bus.estimatedArrival);
                    QString eta = (mins >= 0) ? QString("%1m").arg(mins) : QStringLiteral("Arr");
                    QString loadStr = info.bus.load.isEmpty() ? QStringLiteral("SEA") : info.bus.load;
                    QString typeStr = info.bus.type.isEmpty() ? QStringLiteral("SD") : info.bus.type;
                    QString label = QString("Svc %1 (%2): %3 [%4,%5]")
                                        .arg(info.serviceNo)
                                        .arg(info.labelPrefix)
                                        .arg(eta)
                                        .arg(loadStr)
                                        .arg(typeStr);
                    renderText(x + 8, y + 5, label, QString("Tahoma"));
                }
            }
        }
    }

    if (SCALE > 0.6)
    {
        m_sgBuilding.drawText(layerContext);
        m_sgLandUsages.drawText(layerContext);
        m_sgMotorWays.drawText(layerContext);
        m_sgMainRoads.drawText(layerContext);
        m_sgMinorRoads.drawText(layerContext);
        m_sgMRT.drawText(layerContext);
    }

    if (SCALE > 0.1)
    {
        m_sgWaterArea.drawText(layerContext);
        m_sgAmenities.drawText(layerContext);
        m_sgPlaces.drawText(layerContext);
        m_sgManMade.drawText(layerContext);
    }

    // Live airflight callsign labels (callsign + altitude on two lines, to
    // the right of each marker). The trails and markers are drawn in the
    // polygon/line passes above.
    m_sgFlightMarkers.drawText(layerContext);

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
        glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(m_posAttr);
        glDrawArrays(GL_TRIANGLE_FAN, 0, granularity);
        m_program->setUniformValue(m_colorId, 3);
        glDrawArrays(GL_LINE_STRIP, 0, granularity + 1);
        glDisableVertexAttribArray(m_posAttr);

        float angle = 0;
        if (m_iMousePosX != m_iMouseInitX)
        {
            angle =
                (m_iMousePosX - m_iMouseInitX) >= 0
                    ? atan((float)(m_iMousePosY - m_iMouseInitY) / (float)(m_iMousePosX - m_iMouseInitX)) / 3.1416 * 180
                          + 90
                    : atan((float)(m_iMousePosY - m_iMouseInitY) / (float)(m_iMousePosX - m_iMouseInitX)) / 3.1416 * 180
                          + 270;
        }
        if (m_bMouseIsPressing)
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(l_vertexBuffer2), l_vertexBuffer2, GL_DYNAMIC_DRAW);
            m_program->setUniformValue(m_colorId, 3);
            glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(m_posAttr);
            glDrawArrays(GL_LINES, 0, 2);
            glDisableVertexAttribArray(m_posAttr);

            renderText(
                m_iMousePosX * devicePixelRatio() + 15, m_iMousePosY * devicePixelRatio() + 20,
                QString(tr("Angle: %1, Dist: %2")).arg(angle, 5, 'f', 1, QChar('0')).arg(r, 6, 'f', 1, QChar('0')),
                QString("Courier"));
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}
//! [6]

void TSDWindow::drawMRTStation()
{
    if (!(m_displayMask & MRT_POINT))
    {
        return;
    }

    struct MrtStationGroup
    {
        int colorId;
        int firstVertex;
        int vertexCount;
    };
    static const MrtStationGroup kGroups[] = {
        {5, 0, 29}, {3, 29, 25}, {5, 54, 3}, {9, 57, 16}, {7, 73, 31}, {7, 104, 3}, {19, 107, 35}, {21, 142, 31},
    };

    glPointSize(12);
    glBindBuffer(GL_ARRAY_BUFFER, m_mrtVBO);
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(m_posAttr);

    for (const MrtStationGroup& group : kGroups)
    {
        m_program->setUniformValue(m_colorId, group.colorId);
        glDrawArrays(GL_POINTS, group.firstVertex, group.vertexCount);
    }

    glDisableVertexAttribArray(m_posAttr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Receives the live airflight tracking table from the TrackerWorker (emitted
// from the worker thread, queued to the GUI thread). We copy it into a
// member, feed it to the flight layer parsers, and mark the flight layers
// dirty so render() rebuilds their geometry (where the GL context is current).
void TSDWindow::onTrackingTableUpdated(const QHash<QString, TrackedAircraft>& table)
{
    m_flightTable = table;
    if (m_sgFlightTrails.m_parser)
    {
        static_cast<FlightLayerParser*>(m_sgFlightTrails.m_parser)->setTable(table);
    }
    if (m_sgFlightMarkers.m_parser)
    {
        static_cast<FlightLayerParser*>(m_sgFlightMarkers.m_parser)->setTable(table);
    }
    m_sgFlightTrails.markDirty();
    m_sgFlightMarkers.markDirty();
}

// Rebuild the live flight layers (trails + markers) from the latest tracking
// table and upload them to the GPU. Called from render() (where the GL context
// is current) whenever the tracking table has changed. The base property is the
// coastal layer's, so the flight geometry lands in the same map space as the
// rest of the map.
void TSDWindow::rebuildLiveLayers()
{
    if (m_sgBusRouteLines.isDirty() || m_sgBusRouteLines2.isDirty() || m_sgBusStops.isDirty()
        || m_sgBusStops2.isDirty())
    {
        rebuildBusRouteLayers();
    }

    if (m_sgBusVehicles.isDirty() || m_sgBusWindshields.isDirty())
    {
        const std::array<LiveMapLayer*, 2> vehicleLayers = {&m_sgBusVehicles, &m_sgBusWindshields};
        for (LiveMapLayer* layer : vehicleLayers)
        {
            auto* parser = static_cast<BusLayerParser*>(layer->parser());
            parser->setRoutes(m_activeBusRoutes);
            parser->setSnapshot(m_currentBusStopSnapshot);
        }
    }

    for (LiveMapLayer* layer : m_liveLayers)
    {
        layer->rebuild(m_sgCoastal.m_property, m_fScaleFactor);
    }
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
    {
        m_displayMask |= layer;
    }
    else
    {
        m_displayMask &= ~layer;
    }
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

    // Shader programs are parented to this window, but released here while
    // their GL context is still current.
    for (QOpenGLShaderProgram** programSlot : m_programSlots)
    {
        delete *programSlot;
        *programSlot = nullptr;
    }

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

    for (BaseMapLayer* layer : m_baseLayers)
        layer->releaseGpuResources();
    for (StaticMapLayer* layer : m_staticLayers)
        layer->releaseGpuResources();
    for (LiveMapLayer* layer : m_liveLayers)
        layer->releaseGpuResources();
}

void TSDWindow::fetchBusRoute(const QString& busNo, const QString& accountKey)
{
    BusRouteWorker* busRouteWorker = workerAt<BusRouteWorker>(m_workers, BusRouteWorkerIndex);
    QThread* busRouteThread = m_workers[BusRouteWorkerIndex]->thread();
    if (busNo.isEmpty() || !busRouteWorker || !busRouteThread)
    {
        return;
    }

    if (!accountKey.trimmed().isEmpty())
    {
        m_accountKey = accountKey.trimmed();
    }

    if (!busRouteThread->isRunning())
    {
        busRouteThread->start();
    }

    const QString targetBus = busNo.trimmed();
    const QString key = m_accountKey;
    qWarning().noquote() << "Fetching route for bus service:" << targetBus;

    QMetaObject::invokeMethod(
        busRouteWorker, [busRouteWorker, targetBus, key]() { busRouteWorker->fetchRouteForBus(targetBus, key); },
        Qt::QueuedConnection);
}

void TSDWindow::onBusRouteReady(const QString& busNo, const QList<BusRoute>& routes)
{
    m_currentBusNo = busNo;

    // Append routes for busNo (replacing any existing route entry for the same busNo)
    for (int i = m_activeBusRoutes.size() - 1; i >= 0; --i)
    {
        if (m_activeBusRoutes[i].serviceNo.compare(busNo, Qt::CaseInsensitive) == 0)
        {
            m_activeBusRoutes.removeAt(i);
        }
    }
    m_activeBusRoutes.append(routes);

    m_sgBusRouteLines.markDirty();
    m_sgBusRouteLines2.markDirty();
    m_sgBusStops.markDirty();
    m_sgBusStops2.markDirty();
    renderLater();
}

void TSDWindow::rebuildBusRouteLayers()
{
    std::array<QList<BusRoute>, 2> routesByDirection;
    for (const BusRoute& route : m_activeBusRoutes)
    {
        if (route.direction == 1 || route.direction == 2)
        {
            routesByDirection[route.direction - 1].append(route);
        }
    }

    const std::array<LiveMapLayer*, 4> routeLayers = {&m_sgBusRouteLines, &m_sgBusRouteLines2, &m_sgBusStops,
                                                      &m_sgBusStops2};
    for (size_t index = 0; index < routeLayers.size(); ++index)
    {
        auto* parser = static_cast<BusLayerParser*>(routeLayers[index]->parser());
        parser->setRoutes(routesByDirection[index % 2]);
        routeLayers[index]->markDirty();
    }
}

void TSDWindow::clearBusInfo()
{
    if (BusTrackerWorker* busWorker = workerAt<BusTrackerWorker>(m_workers, BusWorkerIndex))
    {
        QMetaObject::invokeMethod(busWorker, &BusTrackerWorker::stop, Qt::QueuedConnection);
    }
    if (BusRouteWorker* busRouteWorker = workerAt<BusRouteWorker>(m_workers, BusRouteWorkerIndex))
    {
        QMetaObject::invokeMethod(busRouteWorker, &BusRouteWorker::stop, Qt::QueuedConnection);
    }

    m_currentBusNo.clear();
    m_activeBusRoutes.clear();
    const std::array<LiveMapLayer*, 4> routeLayers = {&m_sgBusRouteLines, &m_sgBusRouteLines2, &m_sgBusStops,
                                                      &m_sgBusStops2};
    for (LiveMapLayer* layer : routeLayers)
    {
        static_cast<BusLayerParser*>(layer->parser())->setRoutes({});
        layer->markDirty();
    }

    m_currentBusStopSnapshot = BusStopSnapshot();
    const std::array<LiveMapLayer*, 2> vehicleLayers = {&m_sgBusVehicles, &m_sgBusWindshields};
    for (LiveMapLayer* layer : vehicleLayers)
    {
        static_cast<BusLayerParser*>(layer->parser())->setSnapshot(m_currentBusStopSnapshot);
        layer->markDirty();
    }

    renderLater();
}

void TSDWindow::trackBusStop(const QString& stopCode, const QString& accountKey)
{
    BusTrackerWorker* busWorker = workerAt<BusTrackerWorker>(m_workers, BusWorkerIndex);
    const std::array<QThread*, 2> busThreads = {m_workers[BusWorkerIndex]->thread(),
                                                m_workers[BusRouteWorkerIndex]->thread()};
    if (stopCode.isEmpty() || !busWorker || !busThreads[0])
    {
        return;
    }

    if (!accountKey.trimmed().isEmpty())
    {
        m_accountKey = accountKey.trimmed();
    }

    for (QThread* thread : busThreads)
    {
        if (thread && !thread->isRunning())
        {
            thread->start();
        }
    }

    m_activeBusRoutes.clear();
    const std::array<LiveMapLayer*, 4> routeLayers = {&m_sgBusRouteLines, &m_sgBusRouteLines2, &m_sgBusStops,
                                                      &m_sgBusStops2};
    for (LiveMapLayer* layer : routeLayers)
    {
        layer->markDirty();
    }

    const QString targetStop = stopCode.trimmed();
    const QString key = m_accountKey;
    qWarning().noquote() << "Tracking bus stop:" << targetStop;

    QMetaObject::invokeMethod(
        busWorker, [busWorker, targetStop, key]() { busWorker->trackBusStop(targetStop, key, 15000); },
        Qt::QueuedConnection);
}

void TSDWindow::onBusArrivalUpdated(const BusStopSnapshot& snapshot)
{
    m_currentBusStopSnapshot = snapshot;
    m_sgBusVehicles.markDirty();
    m_sgBusWindshields.markDirty();

    // Automatically fetch and draw routes for all bus services arriving at this bus stop
    for (const BusService& service : snapshot.services)
    {
        if (!service.serviceNo.isEmpty())
        {
            fetchBusRoute(service.serviceNo, m_accountKey);
        }
    }

    renderLater();
}
