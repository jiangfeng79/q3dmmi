#ifndef TSDWINDOW_H
#define TSDWINDOW_H

#include "OpenglWindow.h"
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QScreen>
#include <QMouseEvent>
#include <QPainter>
#include <QOpenGLPaintDevice>
#include <time.h>
#include <qmath.h>
#include <QApplication>
#include <QLabel>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QActionGroup>
#include <QThread>
#include <QHash>

#include "layerGeometry.h"
#include "layerParser.h"
#include "flightTracker.h"

class TSDWindow : public OpenglWindow
{
    Q_OBJECT

public:
    enum DisplayMaskBits
    {
        DISPLAY_VOID = 0,
        COASTAL = 1, //polygon
        COASTAL_TEXT = 1 << 1, //polygon
        PLACES = 1 << 2, //points
        PLACES_TEXT = 1 << 3,
        AMENITIES = 1 << 4, //points
        AMENITIES_TEXT = 1 << 5,
        LAND_USAGE = 1 << 6, //polygon
        LAND_USAGE_TEXT = 1 << 7,
        WATER_AREA = 1 << 8, //polygon
        WATER_AREA_TEXT = 1 << 9,
        BUILDING = 1 << 10, //polygon
        BUILDING_TEXT = 1 << 11,
        MAIN_ROADS = 1 << 12, //lines
        MAIN_ROADS_TEXT = 1 << 13,
        MINOR_ROADS = 1 << 14, //lines
        MINOR_ROADS_TEXT = 1 << 15,
        MOTOR_WAYS = 1 << 16, //lines
        MOTOR_WAYS_TEXT = 1 << 17,
        AIR_WAYS = 1 << 18, //lines
        AIR_WAYS_TEXT = 1 << 19,
        MRT = 1 << 20, //lines
        MRT_TEXT = 1 << 21,
        MAN_MADE = 1 << 22,
        MAN_MADE_TEXT = 1 << 23,

        MRT_POINT = 1 << 24, //points
        FLIGHTS = 1 << 25, //live airflight markers (points)
        FLIGHTS_TEXT = 1 << 26, //live airflight callsign labels
        DIPLAY_ALL = 0xFFFFFFFF
    };

    unsigned int myLog2(unsigned int value)
    {
        unsigned int targetlevel = 0;
        while (value >>= 1) ++targetlevel;
        return targetlevel;
    }

    //template<int N> void printMrtStringToScreen();
    //template<> void printMrtStringToScreen<0>();

    // MapProperty is defined in layerGeometry.h.

    // A map layer is coupled to a single input layer through an injected
    // LayerParser. The parser turns the raw input (shapefile + dbf, or any
    // other source) into renderable geometry (polygons, lines, points and
    // labels) held in m_geometry. The renderer consumes m_geometry directly
    // and never touches the raw file format.
    class MapLayer
    {
    public:
        MapLayer(const char* fileName, DisplayMaskBits id, DisplayMaskBits text_id) :
            m_id(id)
            , m_text_id(text_id)
            , m_bToFill(false)
            , m_parser(nullptr)
        {
            m_VBO_ID[0] = m_VBO_ID[1] = 0;
            m_property.scale = 0.1;
            m_fileName = QString(fileName);
            m_layerName.clear();
        }

        MapLayer(const char* fileName, const char* layerName, DisplayMaskBits id, DisplayMaskBits text_id) :
            m_id(id)
            , m_text_id(text_id)
            , m_bToFill(false)
            , m_parser(nullptr)
        {
            m_VBO_ID[0] = m_VBO_ID[1] = 0;
            m_property.scale = 0.1;
            m_fileName = QString(fileName);
            m_layerName = QString(layerName);
        }

        ~MapLayer()
        {
            delete m_parser;
        }

        // Inject the parser coupled to this layer's input data.
        void setParser(LayerParser* a_parser) { m_parser = a_parser; }
        LayerParser* parser() const { return m_parser; }

        MapProperty m_property;
        QString m_fileName;
        QString m_layerName;
        LayerParser* m_parser;
        // Renderable geometry produced by the parser (vertices, rings, types, labels).
        LayerGeometry m_geometry;
        GLuint m_VBO_ID[2];

        DisplayMaskBits m_id;
        DisplayMaskBits m_text_id;

        bool m_bToFill;

        void buildLayer();
        void buildLayer(MapProperty& a_property, int a_iLayerId);
    };

    //template<int N> void printStringToScreen(MapLayer & a_layer);
    //template<> void printStringToScreen<0>(MapLayer & a_layer);

    TSDWindow();
    ~TSDWindow();
    void initialize();
    void render();

    inline int getFps() { return m_fps; }

    //void drawLayer(MapLayer & a_layer, bool a_bFillPolygon = false, int a_iColorId = 0);
    void drawLayerAndFill(MapLayer& a_layer);
    void drawLayer(MapLayer& a_layer);
    // Draw only the line (SHPT_ARC) rings of a layer using the dedicated line
    // shader (m_lineProgram). The caller binds m_lineProgram and sets color_id.
    void drawLayerLines(MapLayer& a_layer);
    // Stencil-fill helpers (shared by drawLayerAndFill)
    void drawPolygonRing(MapLayer& a_layer, int i);
    void drawRingsToStencil(MapLayer& a_layer);
    void drawRingsToColor(MapLayer& a_layer);
    void drawRingFilled(MapLayer& a_layer, int i);
    void drawText(MapLayer& a_layer);
    void drawTextWithAngle(MapLayer& a_layer);
    void drawEBL(float x, float y, float r);
    void drawMRTStation();
    // Draw the live airflight trails (fading position history) + position
    // vectors near Changi. Called in the batched 2D text pass, before the
    // plane markers, so the trails sit behind the planes.
    void drawFlightTrails();
    // Draw the live airflight markers as plane silhouettes (screen space)
    // near Changi. Called in the batched 2D text pass so the planes keep a
    // fixed size regardless of zoom; each is oriented by the aircraft heading.
    void drawFlightMarkers();
    // Draw the live airflight callsign labels (2D text pass) near Changi.
    // Called in the batched 2D text pass.
    void drawFlightLabels();
    void centerMap();
    void setDisplayMask(DisplayMaskBits layer, bool b);
    inline void setAutoZoom(bool value) { m_bAutoZoom = value; }
    inline bool getAutoZoom() { return m_bAutoZoom; }

    inline void setAutoSwing(bool value) { m_bAutoSwing = value; }
    inline bool getAutoSwing() { return m_bAutoSwing; }

    inline void setShaderToys(bool value) { m_bShaderToys = value; }
    inline bool getShaderToys() { return m_bShaderToys; }

public slots:
    // Receives the live airflight tracking table from the TrackerWorker
    // (emitted from the worker thread, queued to the GUI thread).
    void onTrackingTableUpdated(const QHash<QString, TrackedAircraft>& table);

protected:
    // Release all GL resources + CPU-side layer data so initialize() can run
    // again after the context is recreated (e.g. vsync toggle).
    void resetGpuResources() override;
    virtual void selectShader(uint shaderId);

private:
    GLuint loadShader(GLenum type, const char* source);

    GLuint m_vao;
    GLuint m_shader;
    GLuint m_mrtVBO;
    GLuint m_eblVBO;

    QOpenGLShaderProgram* m_program;
    GLuint m_posAttr;
    GLuint m_colAttr;
    GLuint m_colorIdAttr;
    GLuint m_matrixUniform;
    GLuint m_colorId;

    // Background (ShaderToy) shader, drawn as a fullscreen pass before the map.
    QOpenGLShaderProgram* m_bgProgram;
    GLuint m_bgMatrixUniform;
    GLuint m_bgMouse;
    GLuint m_bgMouseDelta;
    GLuint m_bgResolution;
    GLuint m_bgTime;
    GLuint m_bgShaderId;

    // Line shader: separate program (vshader + gshader + fshader) used to draw
    // the line (SHPT_ARC) rings. The geometry shader expands each segment into
    // a filled quad so lines can be thickened beyond the driver's 1px limit.
    QOpenGLShaderProgram* m_lineProgram;
    GLuint m_lineMatrixUniform;
    GLuint m_lineColorId;
    GLuint m_lineResolution;
    GLuint m_lineTime;
   
    MapLayer m_sgCoastal;
    MapLayer m_sgAmenities;
    MapLayer m_sgPlaces;
    MapLayer m_sgLandUsages;
    MapLayer m_sgMRT;
    MapLayer m_sgWaterArea;
    MapLayer m_sgBuilding;
    MapLayer m_sgMainRoads;
    MapLayer m_sgMotorWays;
    MapLayer m_sgMinorRoads;
    MapLayer m_sgAirWays;
    MapLayer m_sgManMade;
    QVector <TSDWindow::MapLayer*> m_listOfLayers;

    GLfloat* m_mrt;

    unsigned int m_displayMask;

    bool m_bAutoZoom;
    bool m_bAutoSwing;
    bool m_bShaderToys;

    // Live airflight tracking near Changi. The worker runs on a dedicated
    // thread and polls the adsb.lol API; the tracking table is copied onto the
    // GUI thread (via onTrackingTableUpdated) and drawn by
    // drawFlightMarkers()/drawFlightLabels().
    QThread* m_flightThread;
    TrackerWorker* m_flightWorker;
    QHash<QString, TrackedAircraft> m_flightTable;
};

#endif // TSDWINDOW_H

