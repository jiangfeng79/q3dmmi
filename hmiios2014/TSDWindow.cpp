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

/*
template<int N> void TSDWindow::printMrtStringToScreen()
{
    printMrtStringToScreen<N-1>();
    renderText( X_WGS84_COORD_TO_SCREEN_COORD(mrt[N*2])+7, Y_WGS84_COORD_TO_SCREEN_COORD(mrt[N*2+1])+5,
QString(mrt_name[N]), QString("Tahoma"));
}

template<> void TSDWindow::printMrtStringToScreen<0>()
{
    renderText(X_WGS84_COORD_TO_SCREEN_COORD(mrt[0])+7, Y_WGS84_COORD_TO_SCREEN_COORD(mrt[1])+5, QString(mrt_name[0]),
QString("Tahoma"));
}

template<int N> void TSDWindow::printStringToScreen(MapLayer & a_layer)
{
    printStringToScreen<N-1>(a_layer);

    int x = X_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[N].coordinate[0]);
    int y =  Y_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[N].coordinate[1]);
    char * lable = (a_layer.m_dbfFileReader.getEntity())[N].stringValue;

    if(x>0&&x<width() &&y>0&&y<height() &&lable!=NULL)
    {
        //qDebug() << "data:" << i << "x:" << (m_sgWaterArea.m_dbfFileReader.getEntity())[i].coordinate[0] <<"y:"
<<(m_sgWaterArea.m_dbfFileReader.getEntity())[i].coordinate[1] << "value:"
<<(m_sgWaterArea.m_dbfFileReader.getEntity())[i].stringValue; renderText(x,y, QString(lable), QString("Tahoma"));
    }
}

template<> void TSDWindow::printStringToScreen<0>(MapLayer & a_layer)
{
    int x = X_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[0].coordinate[0]);
    int y =  Y_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[0].coordinate[1]);
    char * lable = (a_layer.m_dbfFileReader.getEntity())[0].stringValue;

    if(x>0&&x<width() &&y>0&&y<height() &&lable!=NULL)
    {
        //qDebug() << "data:" << i << "x:" << (m_sgWaterArea.m_dbfFileReader.getEntity())[i].coordinate[0] <<"y:"
<<(m_sgWaterArea.m_dbfFileReader.getEntity())[i].coordinate[1] << "value:"
<<(m_sgWaterArea.m_dbfFileReader.getEntity())[i].stringValue; renderText(x,y, QString(lable), QString("Tahoma"));
    }
}
*/

TSDWindow::TSDWindow()
    : m_program(0),
      m_bgProgram(0),
      m_lineProgram(0),
      m_bgMouse(0),
      m_bgMouseDelta(0),
      m_bgResolution(0),
      m_bgTime(0),
      m_bgShaderId(1),
      m_sgCoastal("./sgMap/singapore", COASTAL, COASTAL_TEXT, true),
      m_sgAmenities("./sgMap/singapore.osm-amenities", AMENITIES, AMENITIES_TEXT),
      m_sgLandUsages("./sgMap/singapore.osm-landusages", LAND_USAGE, LAND_USAGE_TEXT),
      m_sgPlaces("./sgMap/singapore.osm-places", PLACES, PLACES_TEXT),
      m_sgMRT("./sgMap/railways", MRT, MRT_TEXT),
      m_sgWaterArea("./sgMap/singapore.osm-waterareas", WATER_AREA, WATER_AREA_TEXT, true),
      m_sgBuilding("./sgMap/singapore.osm-buildings", BUILDING, BUILDING_TEXT),
      m_sgMainRoads("./sgMap/singapore.osm-mainroads", MAIN_ROADS, MAIN_ROADS_TEXT),
      m_sgMotorWays("./sgMap/singapore.osm-motorways", MOTOR_WAYS, MOTOR_WAYS_TEXT),
      m_sgMinorRoads("./sgMap/singapore.osm-minorroads", MINOR_ROADS, MINOR_ROADS_TEXT),
      m_sgAirWays("./sgMap/singapore.osm-aeroways", AIR_WAYS, AIR_WAYS_TEXT),
      m_sgManMade("./sgMap/singapore.osm-polygon", "man_made", MAN_MADE, MAN_MADE_TEXT, true),
      m_sgFlightTrails(FLIGHT_TRAILS, FLIGHTS_TEXT, false, true),
      m_sgFlightMarkers(FLIGHTS, FLIGHTS_TEXT, true, true),
      m_sgBusRouteLines(BUS_ROUTES, BUS_ROUTES_TEXT, false, true),
      m_sgBusRouteLines2(BUS_ROUTES2, BUS_ROUTES_TEXT, false, true),
      m_sgBusStops(BUS_STOPS, BUS_STOPS_TEXT, false, true),
      m_sgBusStops2(BUS_STOPS2, BUS_STOPS_TEXT, false, true),
      m_sgBusVehicles(BUS_TRACKS, BUS_TRACKS_TEXT, true, true),
      m_sgBusWindshields(BUS_TRACKS_WINDSHIELD, BUS_STOPS_TEXT, true, true),
      m_mrt(nullptr),
      m_mrtVBO(0),
      m_eblVBO(0),
      m_shader(0),
      m_displayMask(0x3DFF5C032B),
      m_bAutoZoom(false),
      m_bAutoSwing(false),
      m_bShaderToys(false)
{
    m_programSlots = {&m_program, &m_bgProgram, &m_lineProgram};

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

    // Live airflight layers: the parser is fed the tracking table each poll
    // (see onTrackingTableUpdated) and rebuilt into geometry on the GUI thread.
    m_sgFlightTrails.setParser(new FlightLayerParser(FlightLayerParser::Trails));
    m_sgFlightMarkers.setParser(new FlightLayerParser(FlightLayerParser::Markers));

    // Live bus layers: the parser is fed bus routes or bus arrival snapshots.
    m_sgBusRouteLines.setParser(new BusLayerParser(BusLayerParser::RouteLines));
    m_sgBusRouteLines2.setParser(new BusLayerParser(BusLayerParser::RouteLines));
    m_sgBusStops.setParser(new BusLayerParser(BusLayerParser::RouteStops));
    m_sgBusStops2.setParser(new BusLayerParser(BusLayerParser::RouteStops));
    m_sgBusVehicles.setParser(new BusLayerParser(BusLayerParser::Vehicles));
    m_sgBusWindshields.setParser(new BusLayerParser(BusLayerParser::VehicleWindshields));

    m_listOfLayers << &m_sgCoastal << &m_sgWaterArea << &m_sgMRT << &m_sgAmenities << &m_sgLandUsages << &m_sgPlaces
                   << &m_sgBuilding << &m_sgMainRoads << &m_sgMotorWays << &m_sgMinorRoads << &m_sgAirWays
                   << &m_sgManMade << &m_sgFlightTrails << &m_sgFlightMarkers << &m_sgBusRouteLines
                   << &m_sgBusRouteLines2 << &m_sgBusStops << &m_sgBusStops2 << &m_sgBusVehicles << &m_sgBusWindshields;

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

void MapLayer::buildLayer(MapProperty& a_property, int a_iLayerDepth)
{
    if (!m_parser)
    {
        return;
    }

    LayerParser::Options options;
    options.baseProperty = a_property;
    options.layerDepth = a_iLayerDepth;
    options.isBaseLayer = false;
    options.useWgs84BuildTransform = (m_id == TSDWindow::MAN_MADE || m_id == TSDWindow::MRT);

    m_geometry = m_parser->parse(options);
    m_property = m_geometry.property;
}

void MapLayer::buildLayer()
{
    if (!m_parser)
    {
        return;
    }

    LayerParser::Options options;
    options.layerDepth = 0;
    options.isBaseLayer = true;
    options.useWgs84BuildTransform = false;

    m_geometry = m_parser->parse(options);
    m_property = m_geometry.property;
}

void TSDWindow::drawTextWithAngle(MapLayer& a_layer)
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
                renderText(x, y, QString::fromStdString(labels[i].text), QString("Tahoma"), labels[i].angle);
            }
        }
    }
}

void TSDWindow::drawText(MapLayer& a_layer)
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

// Draw the live airflight labels: the callsign and the barometric altitude
// on two lines, positioned to the right of each airplane marker.
void TSDWindow::drawFlightText(MapLayer& a_layer)
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
                // Split "CALLSIGN ALT" into two lines, drawn to the right of
                // the marker (fixed screen size, so a fixed pixel offset works).
                const QString full = QString::fromStdString(labels[i].text);
                const int sp = full.indexOf(' ');
                const QString line1 = (sp >= 0) ? full.left(sp) : full;
                const QString line2 = (sp >= 0) ? full.mid(sp + 1) : QString();

                // renderText() centers on its x, so to left-align both lines on
                // the same edge, offset each by half of its own advance.
                const int px = 12 * devicePixelRatio();
                FontEntry* fe = getCachedFont(QStringLiteral("Tahoma"), px, false);
                const QFontMetricsF& metrics = fe->metrics;
                const int left = x + 10 * retinaScale;
                renderText(left + (int)(metrics.horizontalAdvance(line1) / 2), y - 30, line1, QString("Tahoma"));
                if (!line2.isEmpty())
                {
                    renderText(left + (int)(metrics.horizontalAdvance(line2) / 2), y - 30 + px, line2,
                               QString("Tahoma"));
                }
            }
        }
    }
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

    for (int i = 0; i < m_listOfLayers.size(); ++i)
    {
        MapLayer* l_layer = m_listOfLayers[i];
        if (i == 0)
        {
            l_layer->buildLayer();  // sg coastal; base layer
        }
        else
        {
            l_layer->buildLayer(m_sgCoastal.m_property, 0);
        }

        if (!l_layer->m_geometry.vertices.empty())
        {
            glGenBuffers(2, l_layer->m_VBO_ID);
            glBindBuffer(GL_ARRAY_BUFFER, l_layer->m_VBO_ID[0]);
            glBufferData(GL_ARRAY_BUFFER, l_layer->m_geometry.vertices.size() * 3 * sizeof(float),
                         l_layer->m_geometry.vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // if(l_layer->m_geometry.renderType[0] == SHPT_ARC)
            {
                // Create and upload to Element Array Buffer (EBO)
                if (!l_layer->m_VBO_ID[1])
                {
                    glGenBuffers(1, &l_layer->m_VBO_ID[1]);
                }

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, l_layer->m_VBO_ID[1]);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, l_layer->m_geometry.lineIndices.size() * sizeof(GLuint),
                             l_layer->m_geometry.lineIndices.data(), GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
        }
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

//! [4]

// Draw a single polygon ring as a triangle fan. Assumes the VBO is bound and
// the vertex attribute pointer is set up by the caller's surrounding state.
void TSDWindow::drawPolygonRing(MapLayer& a_layer, int i)
{
    const int vertCount = a_layer.m_geometry.rings[i + 1] - a_layer.m_geometry.rings[i];
    if (vertCount < 3)
    {
        return;  // a triangle fan needs at least 3 vertices
    }
    glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
    glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)(a_layer.m_geometry.rings[i] * 3 * 4));
    glEnableVertexAttribArray(m_posAttr);
    glDrawArrays(GL_TRIANGLE_FAN, 0, vertCount);
    glDisableVertexAttribArray(m_posAttr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// PASS 1: write every polygon ring into the stencil buffer (color writes off).
// GL_INVERT toggles the bit, so overlapping rings cancel out (even-odd rule).
void TSDWindow::drawRingsToStencil(MapLayer& a_layer)
{
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  // disable writing to color buffer
    glStencilFunc(GL_ALWAYS, 0x1, 0x1);
    glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);

    for (int i = 0; i < (int)a_layer.m_geometry.rings.size() - 1; ++i)
    {
        if (a_layer.m_geometry.renderType[i] == SHPT_POLYGON)
        {
            drawPolygonRing(a_layer, i);
        }
    }
}

// PASS 2: color the pixels whose stencil bit ended up set (odd coverage).
void TSDWindow::drawRingsToColor(MapLayer& a_layer)
{
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);  // enable writing to color buffer
    glStencilFunc(GL_EQUAL, 0x1, 0x1);                // test if it is odd(1)
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    for (int i = 0; i < (int)a_layer.m_geometry.rings.size() - 1; ++i)
    {
        if (a_layer.m_geometry.renderType[i] == SHPT_POLYGON)
        {
            drawPolygonRing(a_layer, i);
        }
    }
}

// Fill a single ring: stencil pass then color pass, resolved immediately.
void TSDWindow::drawRingFilled(MapLayer& a_layer)
{
    for (int i = 0; i < (int)a_layer.m_geometry.rings.size() - 1; ++i)
    {
        if (a_layer.m_geometry.renderType[i] == SHPT_POLYGON)
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  // disable writing to color buffer
            glStencilFunc(GL_ALWAYS, 0x1, 0x1);
            glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);
            drawPolygonRing(a_layer, i);

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);  // enable writing to color buffer
            glStencilFunc(GL_EQUAL, 0x1, 0x1);                // test if it is odd(1)
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            drawPolygonRing(a_layer, i);
        }
    }
}

void TSDWindow::drawLayerAndFill(MapLayer& a_layer)
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
        drawRingFilled(a_layer);
    }

    glDisable(GL_STENCIL_TEST);
}

void TSDWindow::drawLayer(MapLayer& a_layer)
{
    if (!a_layer.m_VBO_ID[0] || !a_layer.m_VBO_ID[1])
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
    glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(m_posAttr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a_layer.m_VBO_ID[1]);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFF);
    // ONE SINGLE DRAW CALL FOR HUNDREDS OF THOUSANDS OF LINES:
    if (a_layer.m_geometry.renderType.size() > 0 && a_layer.m_geometry.renderType[0] == SHPT_POLYGON)
    {
        glDrawElements(GL_LINE_STRIP, a_layer.m_geometry.lineIndices.size(), GL_UNSIGNED_INT, 0);
    }
    else if (a_layer.m_geometry.renderType.size() > 0 && a_layer.m_geometry.renderType[0] == SHPT_POINT)
    {
        glPointSize(a_layer.m_id == BUS_STOPS ? 7.0f : 6.0f);
        glDrawElements(GL_POINTS, a_layer.m_geometry.lineIndices.size(), GL_UNSIGNED_INT, 0);
    }

    glDisable(GL_PRIMITIVE_RESTART);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(m_posAttr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw only the line (SHPT_ARC) rings of a layer using the dedicated line
// shader (m_lineProgram). The caller must have bound m_lineProgram and set its
// color_id uniform. The geometry shader expands each segment into a filled
// quad, so lines are thickened beyond the driver's 1px glLineWidth limit.
void TSDWindow::drawLayerLines(MapLayer& a_layer)
{
    if (!a_layer.m_VBO_ID[0] || !a_layer.m_VBO_ID[1]
        || (a_layer.m_geometry.renderType.size() > 0 && a_layer.m_geometry.renderType[0] != SHPT_ARC))
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
    glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(m_posAttr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a_layer.m_VBO_ID[1]);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFF);
    // ONE SINGLE DRAW CALL FOR HUNDREDS OF THOUSANDS OF LINES:
    glDrawElements(GL_LINE_STRIP, a_layer.m_geometry.lineIndices.size(), GL_UNSIGNED_INT, 0);

    glDisable(GL_PRIMITIVE_RESTART);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

    rebuildLiveLayers();

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
    {
        matrix.scale((abs(sin(time * 0.1))) * m_sgCoastal.m_property.scale * m_fScaleFactor);
    }
    else
    {
        matrix.scale(m_sgCoastal.m_property.scale * m_fScaleFactor);
    }
    // matrix.scale( (abs(sin(time*0.1)))*m_sgCoastal.m_property.scale*m_fScaleFactor);

    // center on the new center postion
    // matrix.rotate(1000.0f * time / screen()->refreshRate(), 0, 0, 1);
    if (m_bAutoSwing)
    {
        matrix.rotate(sin(time) * 10, 0, 0, 1);
    }

    QVector<int> shaderRenderLayers = {0, 11};  // COASTAL and MAN_MADE layers are drawn by shaders
    // Precompute which layers are drawn by the shader pass so the main loop
    // below can skip them with an O(1) lookup instead of scanning the list.
    QVector<bool> shaderRendered(m_listOfLayers.size(), false);
    for (int idx : shaderRenderLayers)
    {
        shaderRendered[idx] = true;
    }

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

        foreach (int layerIndex, shaderRenderLayers)
        {
            auto* l_layer = m_listOfLayers[layerIndex];
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
        MapLayer* l_layer = m_listOfLayers[i];

        // When ShaderToys is on, the layers in shaderRenderLayers are already
        // drawn with the ShaderToy background shader in the pass above, so skip
        // them here to avoid overdrawn solid color hiding the pattern.
        if ((m_bShaderToys ? !shaderRendered[i] : true) && (m_displayMask & l_layer->m_id))
        {
            m_program->setUniformValue(m_colorId, myLog2(l_layer->m_id));
            if (l_layer->m_bToFill)
            {
                drawLayerAndFill(*l_layer);
            }
            else
            {
                drawLayer(*l_layer);
            }
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
            MapLayer* l_layer = m_listOfLayers[i];
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

    drawEBL(X_SCREEN_COORD_TO_MAP_COORD(m_iMouseInitX), Y_SCREEN_COORD_TO_MAP_COORD(m_iMouseInitY),
            sqrt(m_iMouseDeltaX * m_iMouseDeltaX + m_iMouseDeltaY * m_iMouseDeltaY) / SCALE);
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
    // beginTextFrame();
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
        // printMrtStringToScreen<sizeof(mrt)/sizeof(GLfloat)/2-1>();
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

    // Live airflight callsign labels (callsign + altitude on two lines, to
    // the right of each marker). The trails and markers are drawn in the
    // polygon/line passes above.
    drawFlightText(m_sgFlightMarkers);

    // endTextFrame();
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
    m_sgFlightTrails.m_dirty = true;
    m_sgFlightMarkers.m_dirty = true;
}

// Rebuild the live flight layers (trails + markers) from the latest tracking
// table and upload them to the GPU. Called from render() (where the GL context
// is current) whenever the tracking table has changed. The base property is the
// coastal layer's, so the flight geometry lands in the same map space as the
// rest of the map.
void TSDWindow::rebuildLiveLayers()
{
    if (m_sgBusRouteLines.m_dirty || m_sgBusRouteLines2.m_dirty || m_sgBusStops.m_dirty || m_sgBusStops2.m_dirty)
    {
        rebuildBusRouteLayers();
    }
    if (m_sgBusVehicles.m_dirty || m_sgBusWindshields.m_dirty)
    {
        rebuildBusTrackerLayers();
    }

    // Parse a flight layer's (already updated) parser and (re)upload its
    // geometry to the GPU. The flight layers are small and change every poll,
    // so GL_DYNAMIC_DRAW is appropriate.
    auto rebuildOne = [this](MapLayer& a_layer) {
        if (!a_layer.m_parser)
        {
            return;
        }

        LayerParser::Options options;
        options.baseProperty = m_sgCoastal.m_property;
        options.layerDepth = 0;
        options.isBaseLayer = false;
        options.useWgs84BuildTransform = true;  // flight input is WGS84

        a_layer.m_geometry = a_layer.m_parser->parse(options);
        a_layer.m_property = a_layer.m_geometry.property;

        if (!a_layer.m_geometry.vertices.empty())
        {
            if (!a_layer.m_VBO_ID[0])
            {
                glGenBuffers(1, &a_layer.m_VBO_ID[0]);
            }
            glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
            glBufferData(GL_ARRAY_BUFFER, a_layer.m_geometry.vertices.size() * 3 * sizeof(float),
                         a_layer.m_geometry.vertices.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Trails (SHPT_ARC) need the element array buffer for the line pass.
            if (a_layer.m_geometry.renderType[0] == SHPT_ARC)
            {
                if (!a_layer.m_VBO_ID[1])
                {
                    glGenBuffers(1, &a_layer.m_VBO_ID[1]);
                }
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a_layer.m_VBO_ID[1]);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, a_layer.m_geometry.lineIndices.size() * sizeof(GLuint),
                             a_layer.m_geometry.lineIndices.data(), GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
        }
    };

    for (MapLayer* layer : m_listOfLayers)
    {
        if (!layer->m_isLive)
        {
            continue;
        }

        if (layer->m_lastScale != m_fScaleFactor)
        {
            layer->m_lastScale = m_fScaleFactor;
            layer->m_parser->setScale(m_fScaleFactor);
            layer->m_dirty = true;
        }

        if (layer->m_dirty)
        {
            rebuildOne(*layer);
            layer->m_dirty = false;
        }
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

    // Per-layer CPU data + VBOs, so initialize() can rebuild everything
    // cleanly (buildLayer() re-parses the input layer from disk).
    auto clearLayer = [this](MapLayer* l_layer) {
        if (l_layer->m_VBO_ID[0] || l_layer->m_VBO_ID[1])
        {
            glDeleteBuffers(2, l_layer->m_VBO_ID);
            l_layer->m_VBO_ID[0] = l_layer->m_VBO_ID[1] = 0;
        }
        if (l_layer->m_parser)
        {
            l_layer->m_parser->freeMemory();
        }
        l_layer->m_geometry.clear();
    };

    for (int i = 0; i < m_listOfLayers.size(); ++i)
    {
        clearLayer(m_listOfLayers[i]);
    }
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

    m_sgBusRouteLines.m_dirty = true;
    m_sgBusRouteLines2.m_dirty = true;
    m_sgBusStops.m_dirty = true;
    m_sgBusStops2.m_dirty = true;
    renderLater();
}

void TSDWindow::rebuildBusRouteLayers()
{
    // Build the vertex VBO, and
    // additionally the index VBO for primitive types that need it (ARC line
    // strips and POINT sprites both use primitive-restart indices; POLYGON
    // fills only need the vertex buffer).
    auto rebuildOne = [this](MapLayer& a_layer) {
        if (!a_layer.m_parser)
        {
            return;
        }

        LayerParser::Options options;
        options.baseProperty = m_sgCoastal.m_property;
        options.layerDepth = 0;
        options.isBaseLayer = false;
        options.useWgs84BuildTransform = true;

        a_layer.m_geometry = a_layer.m_parser->parse(options);
        a_layer.m_property = a_layer.m_geometry.property;

        if (!a_layer.m_geometry.vertices.empty())
        {
            if (!a_layer.m_VBO_ID[0])
            {
                glGenBuffers(1, &a_layer.m_VBO_ID[0]);
            }
            glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
            glBufferData(GL_ARRAY_BUFFER, a_layer.m_geometry.vertices.size() * 3 * sizeof(float),
                         a_layer.m_geometry.vertices.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            if (!a_layer.m_geometry.renderType.empty()
                && (a_layer.m_geometry.renderType[0] == SHPT_ARC || a_layer.m_geometry.renderType[0] == SHPT_POINT))
            {
                if (!a_layer.m_VBO_ID[1])
                {
                    glGenBuffers(1, &a_layer.m_VBO_ID[1]);
                }
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a_layer.m_VBO_ID[1]);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, a_layer.m_geometry.lineIndices.size() * sizeof(GLuint),
                             a_layer.m_geometry.lineIndices.data(), GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
        }
    };

    QList<BusRoute> outboundRoutes;
    QList<BusRoute> returnRoutes;
    for (const BusRoute& route : m_activeBusRoutes)
    {
        if (route.direction == 1)
        {
            outboundRoutes.append(route);
        }
        else if (route.direction == 2)
        {
            returnRoutes.append(route);
        }
    }

    if (m_sgBusRouteLines.m_parser)
    {
        static_cast<BusLayerParser*>(m_sgBusRouteLines.m_parser)->setRoutes(outboundRoutes);
    }
    if (m_sgBusRouteLines2.m_parser)
    {
        static_cast<BusLayerParser*>(m_sgBusRouteLines2.m_parser)->setRoutes(returnRoutes);
    }
    if (m_sgBusStops.m_parser)
    {
        static_cast<BusLayerParser*>(m_sgBusStops.m_parser)->setRoutes(outboundRoutes);
    }
    if (m_sgBusStops2.m_parser)
    {
        static_cast<BusLayerParser*>(m_sgBusStops2.m_parser)->setRoutes(returnRoutes);
    }
    rebuildOne(m_sgBusRouteLines);
    rebuildOne(m_sgBusRouteLines2);
    rebuildOne(m_sgBusStops);
    rebuildOne(m_sgBusStops2);
    qWarning() << "Bus route geometry:" << "direction 1 routes" << outboundRoutes.size() << "vertices"
               << m_sgBusRouteLines.m_geometry.vertices.size() << "indices"
               << m_sgBusRouteLines.m_geometry.lineIndices.size() << "direction 2 routes" << returnRoutes.size()
               << "vertices" << m_sgBusRouteLines2.m_geometry.vertices.size() << "indices"
               << m_sgBusRouteLines2.m_geometry.lineIndices.size();
    m_sgBusRouteLines.m_dirty = false;
    m_sgBusRouteLines2.m_dirty = false;
    m_sgBusStops.m_dirty = false;
    m_sgBusStops2.m_dirty = false;
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
    if (m_sgBusRouteLines.m_parser)
    {
        static_cast<BusLayerParser*>(m_sgBusRouteLines.m_parser)->setRoutes(QList<BusRoute>());
    }
    if (m_sgBusRouteLines2.m_parser)
    {
        static_cast<BusLayerParser*>(m_sgBusRouteLines2.m_parser)->setRoutes(QList<BusRoute>());
    }
    if (m_sgBusStops.m_parser)
    {
        static_cast<BusLayerParser*>(m_sgBusStops.m_parser)->setRoutes(QList<BusRoute>());
    }
    if (m_sgBusStops2.m_parser)
    {
        static_cast<BusLayerParser*>(m_sgBusStops2.m_parser)->setRoutes(QList<BusRoute>());
    }
    m_sgBusRouteLines.m_dirty = true;
    m_sgBusRouteLines2.m_dirty = true;
    m_sgBusStops.m_dirty = true;
    m_sgBusStops2.m_dirty = true;

    m_currentBusStopSnapshot = BusStopSnapshot();
    if (m_sgBusVehicles.m_parser)
    {
        static_cast<BusLayerParser*>(m_sgBusVehicles.m_parser)->setSnapshot(BusStopSnapshot());
    }
    m_sgBusVehicles.m_dirty = true;
    m_sgBusWindshields.m_dirty = true;

    renderLater();
}

void TSDWindow::trackBusStop(const QString& stopCode, const QString& accountKey)
{
    BusTrackerWorker* busWorker = workerAt<BusTrackerWorker>(m_workers, BusWorkerIndex);
    QThread* busThread = m_workers[BusWorkerIndex]->thread();
    QThread* busRouteThread = m_workers[BusRouteWorkerIndex]->thread();
    if (stopCode.isEmpty() || !busWorker || !busThread)
    {
        return;
    }

    if (!accountKey.trimmed().isEmpty())
    {
        m_accountKey = accountKey.trimmed();
    }

    if (!busThread->isRunning())
    {
        busThread->start();
    }

    if (busRouteThread && !busRouteThread->isRunning())
    {
        busRouteThread->start();
    }

    // Reset active bus routes when tracking a new bus stop
    m_activeBusRoutes.clear();
    m_sgBusRouteLines.m_dirty = true;
    m_sgBusRouteLines2.m_dirty = true;
    m_sgBusStops.m_dirty = true;
    m_sgBusStops2.m_dirty = true;

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
    m_sgBusVehicles.m_dirty = true;
    m_sgBusWindshields.m_dirty = true;

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

void TSDWindow::rebuildBusTrackerLayers()
{
    auto rebuildOne = [this](MapLayer& a_layer) {
        if (!a_layer.m_parser)
        {
            return;
        }

        LayerParser::Options options;
        options.baseProperty = m_sgCoastal.m_property;
        options.layerDepth = 0;
        options.isBaseLayer = false;
        options.useWgs84BuildTransform = true;

        a_layer.m_geometry = a_layer.m_parser->parse(options);
        a_layer.m_property = a_layer.m_geometry.property;

        if (!a_layer.m_geometry.vertices.empty())
        {
            if (!a_layer.m_VBO_ID[0])
            {
                glGenBuffers(1, &a_layer.m_VBO_ID[0]);
            }
            glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
            glBufferData(GL_ARRAY_BUFFER, a_layer.m_geometry.vertices.size() * 3 * sizeof(float),
                         a_layer.m_geometry.vertices.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    };

    if (m_sgBusVehicles.m_parser)
    {
        auto* busParser = static_cast<BusLayerParser*>(m_sgBusVehicles.m_parser);
        busParser->setRoutes(m_activeBusRoutes);
        busParser->setSnapshot(m_currentBusStopSnapshot);
    }

    if (m_sgBusWindshields.m_parser)
    {
        auto* windshieldParser = static_cast<BusLayerParser*>(m_sgBusWindshields.m_parser);
        windshieldParser->setRoutes(m_activeBusRoutes);
        windshieldParser->setSnapshot(m_currentBusStopSnapshot);
    }

    rebuildOne(m_sgBusVehicles);
    rebuildOne(m_sgBusWindshields);
    m_sgBusVehicles.m_dirty = false;
    m_sgBusWindshields.m_dirty = false;
}
