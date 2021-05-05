#include "TSDWindow.h"

#include <time.h>//or #include<ctime> for time function
#include <stdlib.h>//or #include<cstdlib> for srand function.
#include <random>
#include <QPointF>
#include <QVector>
#include <cstdlib>
#include <ctime>
#include "hmiios2014.h"
#include "mrt.h"

#define WGS84_TO_WGS84WEBMERCATOR(Y) (log(tan((90.0 + (Y)) * M_PI / 360.0)) / (M_PI / 180.0)) //convert from long/lat to google mercator (or EPSG:4326 to EPSG:900913)
#define WGS84WEBMERCATOR_TO_WGS84(Y) (atan(pow(M_E, (Y)*M_PI/180.0))*360.0/M_PI-90.0)
// screen position to map position
//#define X_SCREEN_COORD_TO_MAP_COORD(X) -m_fMapCenterDeltaX/m_property.scale+((X)-width()/2)/(m_property.scale*m_fScaleFactor)
//#define Y_SCREEN_COORD_TO_MAP_COORD(Y) m_fMapCenterDeltaY/m_property.scale-((Y)-height()/2)/(m_property.scale*m_fScaleFactor)
#define X_SCREEN_COORD_TO_MAP_COORD(X) -m_fMapCenterDeltaX/m_sgCoastal.m_property.scale+((X)-width()/2)/(m_sgCoastal.m_property.scale*m_fScaleFactor)
#define Y_SCREEN_COORD_TO_MAP_COORD(Y) m_fMapCenterDeltaY/m_sgCoastal.m_property.scale-((Y)-height()/2)/(m_sgCoastal.m_property.scale*m_fScaleFactor)


// lat-long position to map position
#define X_WGS84_BUILD_COORD_TO_MAP_COORD(X) ((X)-m_property.centerX)*m_property.mapBuildScale
#define Y_WGS84_BUILD_COORD_TO_MAP_COORD(Y) (WGS84_TO_WGS84WEBMERCATOR(Y)-m_property.centerY)*m_property.mapBuildScale

#define X_METRE_COORD_TO_MAP_COORD(X) ((X)-m_property.centerX*m_property.mapBuildScale) // for unit metre
#define Y_METRE_COORD_TO_MAP_COORD(Y) ((Y)-m_property.centerY*m_property.mapBuildScale)

// screen position to lat-long position
#define X_SCREEN_COORD_TO_WGS84(X) m_sgCoastal.m_property.centerX-m_fMapCenterDeltaX/(m_sgCoastal.m_property.mapBuildScale*m_sgCoastal.m_property.scale)+((X)-width()*devicePixelRatio()/2)/(m_sgCoastal.m_property.mapBuildScale*m_sgCoastal.m_property.scale*m_fScaleFactor)
#define Y_SCREEN_COORD_TO_WGS84(Y) WGS84WEBMERCATOR_TO_WGS84(m_sgCoastal.m_property.centerY+m_fMapCenterDeltaY/(m_sgCoastal.m_property.mapBuildScale*m_sgCoastal.m_property.scale)-((Y)-height()*devicePixelRatio()/2)/(m_sgCoastal.m_property.mapBuildScale*m_sgCoastal.m_property.scale*m_fScaleFactor))

// lat-long position to screen position, for drawing text
#define X_WGS84_COORD_TO_SCREEN_COORD(X) width()*devicePixelRatio()/2+((X)-m_sgCoastal.m_property.centerX+m_fMapCenterDeltaX/(m_sgCoastal.m_property.mapBuildScale*m_sgCoastal.m_property.scale))*m_sgCoastal.m_property.scale*m_fScaleFactor*m_sgCoastal.m_property.mapBuildScale
#define Y_WGS84_COORD_TO_SCREEN_COORD(Y) height()*devicePixelRatio()/2-(WGS84_TO_WGS84WEBMERCATOR(Y)-m_sgCoastal.m_property.centerY-m_fMapCenterDeltaY/(m_sgCoastal.m_property.mapBuildScale*m_sgCoastal.m_property.scale))*m_sgCoastal.m_property.scale*m_fScaleFactor*m_sgCoastal.m_property.mapBuildScale

#define X_WGS84_COORD_TO_MAP_COORD(X) ((X)-m_sgCoastal.m_property.centerX)*m_sgCoastal.m_property.mapBuildScale
#define Y_WGS84_COORD_TO_MAP_COORD(Y) (WGS84_TO_WGS84WEBMERCATOR(Y)-m_sgCoastal.m_property.centerY)*m_sgCoastal.m_property.mapBuildScale

// scale
#define SCALE (m_sgCoastal.m_property.scale*m_fScaleFactor)

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
    , m_displayMask(0xff5c033f)
    , m_bAutoZoom(false)
    , m_bAutoSwing(false)
{
    m_sgCoastal.m_bToFill = true;
    m_sgWaterArea.m_bToFill = true;
    m_sgManMade.m_bToFill = true;

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

    //bool bOK = connect(&(G_P_MAINWINDOW->UIQueue), SIGNAL(signal_send_msg()), this, SLOT(slot_process_msg()));
    //assert(bOK);
}

TSDWindow::~TSDWindow()
{
    glDeleteLists(m_listIndex, 32);
    free(m_mrt);
}

void TSDWindow::MapLayer::buildLayer(MapProperty& a_property, int a_iLayerDepth)
{
    readData();
    ShpReader::ShpEntity* l_shapeEntity = m_shapeFileReader.getEntity();
    DBFReader::DBFEntity* l_dbfEntity = m_dbfFileReader.getEntity();
    m_property = a_property;

    m_ring.push_back(0);
    int l_iTotalVertex = 0;
    if (l_shapeEntity)
    {
        for (int n = 0; n < m_shapeFileReader.getNumberOfEntity(); ++n)
            m_property.totalNumberOfVertex += l_shapeEntity[n].totalVertex;

        m_vertex = (GLfloat*)malloc(sizeof(GLfloat) * m_property.totalNumberOfVertex * 3);
        if (m_vertex)
        {
            l_iTotalVertex = 0;

            for (int n = 0; n < m_shapeFileReader.getNumberOfEntity(); ++n)
            {
                if (l_dbfEntity)
                {
                    if (m_id == MAN_MADE || m_id == MRT)
                    {
                        l_dbfEntity[n].coordinate[0] = ((l_shapeEntity[n].maxX + l_shapeEntity[n].minX) / 2);
                        l_dbfEntity[n].coordinate[1] = ((l_shapeEntity[n].maxY + l_shapeEntity[n].minY) / 2);
                    }
                    else
                    {
                        l_dbfEntity[n].coordinate[0] = ((l_shapeEntity[n].maxX + l_shapeEntity[n].minX) / 2) / m_property.mapBuildScale;
                        l_dbfEntity[n].coordinate[1] = WGS84WEBMERCATOR_TO_WGS84(((l_shapeEntity[n].maxY + l_shapeEntity[n].minY) / 2) / m_property.mapBuildScale);
                    }
                    if (l_shapeEntity[n].type == SHPT_ARC)
                    {
                        std::srand(std::time(0)); // use current time as seed for random generator
                        int random_variable = std::rand() % (l_shapeEntity[n].totalVertex);
                        if (random_variable == (l_shapeEntity[n].totalVertex - 1)) random_variable--;

                        l_dbfEntity[n].coordinate[0] = (l_shapeEntity[n].coordinate[random_variable][0] + l_shapeEntity[n].coordinate[random_variable + 1][0]) / 2 / m_property.mapBuildScale;
                        l_dbfEntity[n].coordinate[1] = WGS84WEBMERCATOR_TO_WGS84((l_shapeEntity[n].coordinate[random_variable][1] + l_shapeEntity[n].coordinate[random_variable + 1][1]) / 2 / m_property.mapBuildScale);
                        //l_dbfEntity[n].angle = atan2(-l_shapeEntity[n].coordinate[random_variable+1][1]+l_shapeEntity[n].coordinate[random_variable][1],
                        //l_shapeEntity[n].coordinate[random_variable+1][0]-l_shapeEntity[n].coordinate[random_variable][0])/M_PI*180.0;
                        l_dbfEntity[n].angle = atan2(-l_shapeEntity[n].coordinate[random_variable + 1][1] + l_shapeEntity[n].coordinate[random_variable][1],
                            l_shapeEntity[n].coordinate[random_variable + 1][0] - l_shapeEntity[n].coordinate[random_variable][0]) / M_PI * 180.0;
                        //qDebug() << "l_dAngle" << l_dbfEntity[n].angle;
                    }
                }
                l_iTotalVertex += l_shapeEntity[n].totalVertex;
                for (int i = 0; i < l_shapeEntity[n].totalVertex; ++i)
                {
                    if (m_id == MAN_MADE || m_id == MRT)
                    {
                        (m_vertex)[(l_iTotalVertex - l_shapeEntity[n].totalVertex + i) * 3] = X_WGS84_BUILD_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][0]);
                        //double y = log(tan((90.0 + l_shapeEntity[n].coordinate[i][1]) * M_PI / 360.0)) / (M_PI / 180.0);
                        (m_vertex)[(l_iTotalVertex - l_shapeEntity[n].totalVertex + i) * 3 + 1] = Y_WGS84_BUILD_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][1]);
                        //(m_vertex)[(l_iTotalVertex-l_shapeEntity[n].totalVertex+i)*3+1] = Y_WGS84_BUILD_COORD_TO_MAP_COORD(y);
                        (m_vertex)[(l_iTotalVertex - l_shapeEntity[n].totalVertex + i) * 3 + 2] = a_iLayerDepth;
                    }
                    else
                    {
                        (m_vertex)[(l_iTotalVertex - l_shapeEntity[n].totalVertex + i) * 3] = X_METRE_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][0]);
                        (m_vertex)[(l_iTotalVertex - l_shapeEntity[n].totalVertex + i) * 3 + 1] = Y_METRE_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][1]);
                        (m_vertex)[(l_iTotalVertex - l_shapeEntity[n].totalVertex + i) * 3 + 2] = a_iLayerDepth;
                    }

                    if (l_shapeEntity[n].isRing[i] == 1)
                    {
                        m_ring.push_back(i + l_iTotalVertex - l_shapeEntity[n].totalVertex);
                        m_renderType.push_back(l_shapeEntity[n].type);
                    }
                }
                m_ring.push_back(l_iTotalVertex);
                m_renderType.push_back(l_shapeEntity[n].type);
            }
        }
    }
}

void TSDWindow::MapLayer::buildLayer()
{
    readData();
    ShpReader::ShpEntity* l_shapeEntity = m_shapeFileReader.getEntity();
    //m_property.centerX = (m_shapeFileReader.getShpMaxX() + m_shapeFileReader.getShpMinX())/2/m_property.mapBuildScale;
    //m_property.centerY = (m_shapeFileReader.getShpMaxY() + m_shapeFileReader.getShpMinY())/2/m_property.mapBuildScale;
    m_property.centerX = 103.84810;
    m_property.centerY = WGS84_TO_WGS84WEBMERCATOR(1.35059); // this is not latitude! call WGS84_TO_WGS84WEBMERCATOR to make it latitude
    //qDebug() << DEGREE_TO_DEGREE_REVERSE(m_property.centerY) << DEGREE_TO_DEGREE(m_property.centerY);

    m_property.width = (m_shapeFileReader.getShpMaxX() - m_shapeFileReader.getShpMinX());
    m_property.height = (m_shapeFileReader.getShpMaxY() - m_shapeFileReader.getShpMinY());
    //m_property.mapBuildScale = 111319.49079;

    m_property.scale = .02;

    m_ring.push_back(0);
    int l_iTotalVertex = 0;
    if (l_shapeEntity)
    {
        for (int n = 0; n < m_shapeFileReader.getNumberOfEntity(); ++n)
            m_property.totalNumberOfVertex += l_shapeEntity[n].totalVertex;
        m_vertex = (GLfloat*)malloc(sizeof(GLfloat) * m_property.totalNumberOfVertex * 3);
        if (m_vertex)
        {
            l_iTotalVertex = 0;
            for (int n = 0; n < m_shapeFileReader.getNumberOfEntity(); ++n)
            {
                l_iTotalVertex += l_shapeEntity[n].totalVertex;
                for (int i = 0; i < l_shapeEntity[n].totalVertex; ++i)
                {
                    (m_vertex)[(l_iTotalVertex - l_shapeEntity[n].totalVertex + i) * 3] = X_METRE_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][0]);
                    (m_vertex)[(l_iTotalVertex - l_shapeEntity[n].totalVertex + i) * 3 + 1] = Y_METRE_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][1]);
                    (m_vertex)[(l_iTotalVertex - l_shapeEntity[n].totalVertex + i) * 3 + 2] = 0;

                    if (l_shapeEntity[n].isRing[i] == 1)
                    {
                        m_ring.push_back(i + l_iTotalVertex - l_shapeEntity[n].totalVertex);
                        m_renderType.push_back(l_shapeEntity[n].type);
                    }
                }
                m_ring.push_back(l_iTotalVertex);
                m_renderType.push_back(l_shapeEntity[n].type);
            }
        }
    }
}

void TSDWindow::drawTextWithAngle(TSDWindow::MapLayer& a_layer)
{
    const qreal retinaScale = devicePixelRatio();
    if (a_layer.m_text_id & m_displayMask)
    {
        for (int i = 0; i < a_layer.m_dbfFileReader.getNumberOfRecords(); ++i)
        {
            int x = X_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[i].coordinate[0]) * retinaScale;
            int y = Y_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[i].coordinate[1]) * retinaScale;
            char* lable = (a_layer.m_dbfFileReader.getEntity())[i].stringValue;
            if (x > 0 && x < width() * retinaScale && y>0 && y < height() * retinaScale && lable != NULL)
                renderText(x, y, QString(lable), QString("Tahoma"), (a_layer.m_dbfFileReader.getEntity())[i].angle);
        }
    }
}

void TSDWindow::drawText(TSDWindow::MapLayer& a_layer)
{
    const qreal retinaScale = devicePixelRatio();
    if (a_layer.m_text_id & m_displayMask)
    {
        for (int i = 0; i < a_layer.m_dbfFileReader.getNumberOfRecords(); ++i)
        {
            int x = X_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[i].coordinate[0]) * retinaScale;
            int y = Y_WGS84_COORD_TO_SCREEN_COORD((a_layer.m_dbfFileReader.getEntity())[i].coordinate[1]) * retinaScale;
            char* lable = (a_layer.m_dbfFileReader.getEntity())[i].stringValue;
            if (x > 0 && x < width() * retinaScale && y>0 && y < height() * retinaScale && lable != NULL)
            {
                renderText(x, y, QString(lable), QString("Tahoma"));
            }
        }
    }
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

    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, QString(":/hmiios2014/vshader.glsl"));
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(":/hmiios2014/fshader.glsl"));

    m_program->link();

    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");
    m_mouse = m_program->uniformLocation("mouse");
    m_mouseDelta = m_program->uniformLocation("mouseDelta");
    m_resolution = m_program->uniformLocation("resolution");
    m_time = m_program->uniformLocation("time");
    m_colorId = m_program->uniformLocation("color_id");

    m_program->bind();

    for (int i = 0; i < m_listOfLayers.size(); ++i)
    {
        TSDWindow::MapLayer* l_layer = m_listOfLayers[i];
        if (i == 0)
            l_layer->buildLayer(); // sg coastal; base layer
        else
            l_layer->buildLayer(m_sgCoastal.m_property, 0);


        if (l_layer->m_vertex)
        {
            glGenBuffers(2, l_layer->m_VBO_ID);
            glBindBuffer(GL_ARRAY_BUFFER, l_layer->m_VBO_ID[0]);
            glBufferData(GL_ARRAY_BUFFER, l_layer->m_property.totalNumberOfVertex * 3 * 4, l_layer->m_vertex, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            free(l_layer->m_vertex);
            l_layer->m_shapeFileReader.freeMemory();
            //l_layer->m_dbfFileReader.freeMemory();
        }
    }

    m_mrt = (GLfloat*)malloc(sizeof(mrt));
    for (int i = 0; i < sizeof(mrt) / sizeof(GLfloat) / 2; ++i)
    {
        m_mrt[i * 2] = X_WGS84_COORD_TO_MAP_COORD(mrt[i * 2]);
        m_mrt[i * 2 + 1] = Y_WGS84_COORD_TO_MAP_COORD(mrt[i * 2 + 1]);
    }

    //gl list codes
    m_listIndex = glGenLists(32);

    foreach(TSDWindow::MapLayer * l_layer, m_listOfLayers)
    {
        unsigned int layer_id = myLog2(l_layer->m_id);

        glNewList(m_listIndex + layer_id, GL_COMPILE);
        if (l_layer->m_bToFill)
        {
            drawLayerAndFill(*l_layer, layer_id);
            //drawLayer(*l_layer, layer_id);
        }
        else
        {
            //if(l_layer->m_id==LAND_USAGE)
            //	drawLayer(m_sgCoastal,21);
            drawLayer(*l_layer, layer_id);
        }
        glEndList();
    }

    glNewList(m_listIndex + myLog2(MRT_POINT), GL_COMPILE);
    drawMRTStation();
    glEndList();
}
//! [4]

void TSDWindow::drawLayerAndFill(MapLayer& a_layer, int a_iColorId)
{
    // polygon
    glClear(GL_STENCIL_BUFFER_BIT);
    glClearStencil(0x0);
    m_program->setUniformValue(m_colorId, a_iColorId);
    // enable stencil test
    glEnable(GL_STENCIL_TEST);

    if (a_iColorId == myLog2(WATER_AREA))
    {
        // PASS 1: draw to stencil buffer only
        // The reference value will be written to the stencil buffer plane if test passed
        // The stencil buffer is initially set to all 0s.

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // disable writing to color buffer
        glStencilFunc(GL_ALWAYS, 0x1, 0x1);
        glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);

        for (int i = 0; i < a_layer.m_ring.size() - 1; ++i)
        {
            // polygons
            if (a_layer.m_renderType[i] == SHPT_POLYGON)
            {
                glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
                
                glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)(a_layer.m_ring[i] * 3 * 4));
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glEnableVertexAttribArray(0);
                //glDisable(GL_DEPTH_TEST);
                glDrawArrays(GL_TRIANGLE_FAN, 0, a_layer.m_ring[i + 1] - a_layer.m_ring[i]);
                //glEnable(GL_DEPTH_TEST);
                glDisableVertexAttribArray(0);
            }
        }
        // PASS 2: draw color buffer
        // Draw again the exact same polygon to color buffer where the stencil
        // value is only odd number(1). The even(0) area will be descarded.

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);    // enable writing to color buffer
        glStencilFunc(GL_EQUAL, 0x1, 0x1);                  // test if it is odd(1)
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        for (int i = 0; i < a_layer.m_ring.size() - 1; ++i)
        {
            // polygons
            if (a_layer.m_renderType[i] == SHPT_POLYGON)
            {
                glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
                glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)(a_layer.m_ring[i] * 3 * 4));
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glEnableVertexAttribArray(0);
                //glDisable(GL_DEPTH_TEST);
                glDrawArrays(GL_TRIANGLE_FAN, 0, a_layer.m_ring[i + 1] - a_layer.m_ring[i]);
                //glEnable(GL_DEPTH_TEST);
                glDisableVertexAttribArray(0);
            }
        }
    }
    else {

        for (int i = 0; i < a_layer.m_ring.size() - 1; ++i)
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // disable writing to color buffer
            glStencilFunc(GL_ALWAYS, 0x1, 0x1);
            glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);
            // polygons
            if (a_layer.m_renderType[i] == SHPT_POLYGON)
            {
                glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
                glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)(a_layer.m_ring[i] * 3 * 4));
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glEnableVertexAttribArray(0);
                //glDisable(GL_DEPTH_TEST);
                glDrawArrays(GL_TRIANGLE_FAN, 0, a_layer.m_ring[i + 1] - a_layer.m_ring[i]);
                //glEnable(GL_DEPTH_TEST);
                glDisableVertexAttribArray(0);
            }
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);    // enable writing to color buffer
            glStencilFunc(GL_EQUAL, 0x1, 0x1);                  // test if it is odd(1)
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            // polygons
            if (a_layer.m_renderType[i] == SHPT_POLYGON)
            {
                glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
                glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)(a_layer.m_ring[i] * 3 * 4));
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glEnableVertexAttribArray(0);
                //glDisable(GL_DEPTH_TEST);
                glDrawArrays(GL_TRIANGLE_FAN, 0, a_layer.m_ring[i + 1] - a_layer.m_ring[i]);
                //glEnable(GL_DEPTH_TEST);
                glDisableVertexAttribArray(0);
            }
        }
    }
    glDisable(GL_STENCIL_TEST);
}

void TSDWindow::drawLayer(MapLayer& a_layer, int a_iColorId)
{
    for (int i = 0; i < a_layer.m_ring.size() - 1; ++i)
    {
        // polygons
        if (a_layer.m_renderType[i] == SHPT_POLYGON
            /*|| a_layer.m_renderType[i] == SHPT_POLYGONZ
            || a_layer.m_renderType[i] == SHPT_POLYGONM*/)
        {
            glLineWidth(1);
            m_program->setUniformValue(m_colorId, a_iColorId);
            glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
            glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)(a_layer.m_ring[i] * 3 * 4));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_LINE_STRIP, 0, a_layer.m_ring[i + 1] - a_layer.m_ring[i]);
            glDisableVertexAttribArray(0);
        }
        // lines
        else if (a_layer.m_renderType[i] == SHPT_ARC)
        {
            if (a_iColorId == myLog2(MOTOR_WAYS))
                glLineWidth(5);
            else if (a_iColorId == myLog2(MAIN_ROADS))
                glLineWidth(3);
            else if (a_iColorId == myLog2(MRT))
                glLineWidth(3);
            else
                glLineWidth(2);
            m_program->setUniformValue(m_colorId, a_iColorId);
            glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
            glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)(a_layer.m_ring[i] * 3 * 4));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_LINE_STRIP, 0, a_layer.m_ring[i + 1] - a_layer.m_ring[i]);
            glDisableVertexAttribArray(0);
        }
        // points
        else if (a_layer.m_renderType[i] == SHPT_POINT)
        {
            glPointSize(5);
            glLineWidth(2);
            m_program->setUniformValue(m_colorId, a_iColorId);
            glBindBuffer(GL_ARRAY_BUFFER, a_layer.m_VBO_ID[0]);
            glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)(a_layer.m_ring[i] * 3 * 4));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, a_layer.m_ring[i + 1] - a_layer.m_ring[i]);
            glDisableVertexAttribArray(0);
        }
    }
}

//! [5]
void TSDWindow::render()
{
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    int w = width(), h = height();

    //glClearColor(181.0/255.0, 208.0/255.0, 208.0/255.0, 1.0);
    //glClearColor(.0, .0, .35, 1.0);
    //glClearColor(.7, .7, .7, 1.);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
    glEnable(GL_BLEND);
    //glEnable( GL_DEPTH_TEST);
    //glEnable( GL_POINT_SMOOTH);
    //glEnable( GL_POLYGON_SMOOTH);
    //glEnable( GL_LINE_SMOOTH );
    //glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE);

    GLfloat time = (GLfloat)clock() / (GLfloat)CLOCKS_PER_SEC;
    GLfloat mouse[] = { (GLfloat)m_iMousePosX, (GLfloat)m_iMousePosY };
    GLfloat mouseDelta[] = { (GLfloat)m_fMapCenterDeltaX * m_fScaleFactor, (GLfloat)m_fMapCenterDeltaY * m_fScaleFactor };
    GLfloat resolution[] = { (GLfloat)w, (GLfloat)h };

    QMatrix4x4 matrix;
    matrix.ortho(-w / 2, w / 2, -h / 2, h / 2, -500.0, 500.0);

    //matrix.rotate(-m_dRotationAngle/M_PI*180.0, 0, 0, 1);

    matrix.translate((m_fMapCenterDeltaX * m_fScaleFactor), -(m_fMapCenterDeltaY * m_fScaleFactor), 0);

    //l_fScaleFactor =  (abs(sin(time*0.1)));
    if (m_bAutoZoom)
        matrix.scale((abs(sin(time * 0.1))) * m_sgCoastal.m_property.scale * m_fScaleFactor);
    else
        matrix.scale(m_sgCoastal.m_property.scale * m_fScaleFactor);
    //matrix.scale( (abs(sin(time*0.1)))*m_sgCoastal.m_property.scale*m_fScaleFactor);

    //center on the new center postion
    //matrix.rotate(1000.0f * time / screen()->refreshRate(), 0, 0, 1);
    if (m_bAutoSwing)
        matrix.rotate(sin(time) * 10, 0, 0, 1);

    m_program->bind();
    m_program->setUniformValue(m_matrixUniform, matrix);
    m_program->setUniformValue(m_mouse, mouse[0] * devicePixelRatio(), -mouse[1] * devicePixelRatio() + resolution[1]/2);
    m_program->setUniformValue(m_mouseDelta, mouseDelta[0] * devicePixelRatio(), -mouseDelta[1] * devicePixelRatio());
    m_program->setUniformValue(m_resolution, resolution[0], resolution[1]);
    m_program->setUniformValue(m_time, time);

    //glCallList(m_listIndex+m_sgCoastal.m_id);
    //if(SCALE>0.1)
    //glCallList(m_listIndex+2);
    foreach(TSDWindow::MapLayer * l_layer, m_listOfLayers)
    {

        unsigned int layer_id = myLog2(l_layer->m_id);
        if (m_displayMask & l_layer->m_id)
            glCallList(m_listIndex + layer_id);
    }
    glCallList(m_listIndex + myLog2(MRT_POINT));


    drawEBL(X_SCREEN_COORD_TO_MAP_COORD(m_iMouseInitX), Y_SCREEN_COORD_TO_MAP_COORD(m_iMouseInitY), sqrt(m_iMouseDeltaX * m_iMouseDeltaX + m_iMouseDeltaY * m_iMouseDeltaY) / SCALE);

    m_program->release();

    // get mouse pos coord in map
    float mapX = X_SCREEN_COORD_TO_WGS84(m_iMousePosX);
    float mapY = Y_SCREEN_COORD_TO_WGS84(m_iMousePosY);

    //QString X = QString("%1'%2").arg((int)mapX).arg((mapX-(int)mapX)*60.0,6,'f', 3, QChar('0'));
    //QString Y = QString("%1'%2").arg((int)mapY).arg((mapY-(int)mapY)*60.0,6,'f', 3, QChar('0'));
    QString X = QString("%1").arg((mapX), 6, 'f', 6, QChar('0'));
    QString Y = QString("%1").arg((mapY), 6, 'f', 6, QChar('0'));

    //QString X1 = QString("%1'%2").arg((int)mapX).arg((mapX-(int)mapX));
    //QString Y1 = QString("%1'%2").arg((int)mapY).arg((mapY-(int)mapY));

    renderShape(QRect(0, 0, 300 * retinaScale, 80 * retinaScale));
    renderText(10, 18 * retinaScale, QString("Coord: [%1,%2]").arg(X).arg(Y));
    renderText(10, 36 * retinaScale, QString("Scale: [%1]").arg(SCALE));
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
            if (x > 0 && x < width() * retinaScale && y>0 && y < height() * retinaScale)
                renderText(x + 7, y + 5, QString(mrt_name[i]), QString("Tahoma"));
        }
        //printMrtStringToScreen<sizeof(mrt)/sizeof(GLfloat)/2-1>();
    }

    if (SCALE > 0.6)
    {
        drawText(m_sgBuilding);
        //printBuildingStringToScreen<34696-1>();
        //printBuildingStringToScreen<2000-1>();
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
    glLineWidth(1);
#define granularity  63
    m_program->setUniformValue(m_colorId, 16);
    if (m_uiMapOpMask == EBL)
    {
        //EBL
        GLfloat l_vertexBuffer[(granularity + 1) * 2];
        GLfloat l_vertexBuffer2[2 * 2];
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

        glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, l_vertexBuffer);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 63);
        glDisableVertexAttribArray(0);
        m_program->setUniformValue(m_colorId, 3);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_LINE_STRIP, 0, 64);

        glDisableVertexAttribArray(0);

        float angle = 0;
        if (m_iMousePosX != m_iMouseInitX)
            angle = (m_iMousePosX - m_iMouseInitX) >= 0 ? atan((float)(m_iMousePosY - m_iMouseInitY) / (float)(m_iMousePosX - m_iMouseInitX)) / 3.1416 * 180 + 90 : atan((float)(m_iMousePosY - m_iMouseInitY) / (float)(m_iMousePosX - m_iMouseInitX)) / 3.1416 * 180 + 270;
        if (m_bMouseIsPressing)
        {
            glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, l_vertexBuffer2);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_LINES, 0, 2);
            glDisableVertexAttribArray(0);

            renderText(m_iMousePosX * devicePixelRatio() + 15, m_iMousePosY * devicePixelRatio() + 20, QString(tr("Angle: %1, Dist: %2")).arg(angle, 5, 'f', 1, QChar('0')).arg(r), QString("Courier"));
        }
    }
}
//! [6]

void TSDWindow::drawMRTStation()
{
    glPointSize(12);

    m_program->setUniformValue(m_colorId, 5); // EW
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, m_mrt);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINTS, 0, 29);
    glDisableVertexAttribArray(0);

    m_program->setUniformValue(m_colorId, 3);//NS
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)&(m_mrt[29 * 2]));
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINTS, 0, 25);
    glDisableVertexAttribArray(0);

    m_program->setUniformValue(m_colorId, 5); //expo, changi
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)&(m_mrt[54 * 2]));
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINTS, 0, 3);
    glDisableVertexAttribArray(0);

    m_program->setUniformValue(m_colorId, 9); //NE
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)&(m_mrt[57 * 2]));
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINTS, 0, 16);
    glDisableVertexAttribArray(0);

    m_program->setUniformValue(m_colorId, 7); // circle
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)&(m_mrt[73 * 2]));
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINTS, 0, 28);
    glDisableVertexAttribArray(0);

    m_program->setUniformValue(m_colorId, 7); //marina bay
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)&(m_mrt[101 * 2]));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINTS, 0, 7);
    glDisableVertexAttribArray(0);
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
