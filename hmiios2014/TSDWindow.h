#ifndef TSDWINDOW_H
#define TSDWINDOW_H

#include <QActionGroup>
#include <QApplication>
#include <QGuiApplication>
#include <QHash>
#include <QLabel>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLPaintDevice>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QPainter>
#include <QScreen>
#include <QThread>
#include <qmath.h>

#include <array>
#include <functional>
#include <memory>
#include <time.h>
#include <vector>

#include "MapLayer.h"
#include "OpenglWindow.h"
#include "WorkerEntry.h"
#include "busLayerParser.h"
#include "busRoute.h"
#include "busTracker.h"
#include "flightLayerParser.h"
#include "flightTracker.h"

class TSDWindow : public OpenglWindow
{
    Q_OBJECT
    friend class MapLayer;
    friend class BaseMapLayer;
    friend class StaticMapLayer;
    friend class LiveMapLayer;

public:
    enum DisplayMaskBits : std::uint64_t
    {
        COASTAL = 1,            // polygon
        COASTAL_TEXT = 1 << 1,  // polygon
        PLACES = 1 << 2,        // points
        PLACES_TEXT = 1 << 3,
        AMENITIES = 1 << 4,  // points
        AMENITIES_TEXT = 1 << 5,
        LAND_USAGE = 1 << 6,  // polygon
        LAND_USAGE_TEXT = 1 << 7,
        WATER_AREA = 1 << 8,  // polygon
        WATER_AREA_TEXT = 1 << 9,
        BUILDING = 1 << 10,  // polygon
        BUILDING_TEXT = 1 << 11,
        MAIN_ROADS = 1 << 12,  // lines
        MAIN_ROADS_TEXT = 1 << 13,
        MINOR_ROADS = 1 << 14,  // lines
        MINOR_ROADS_TEXT = 1 << 15,
        MOTOR_WAYS = 1 << 16,  // lines
        MOTOR_WAYS_TEXT = 1 << 17,
        AIR_WAYS = 1 << 18,  // lines
        AIR_WAYS_TEXT = 1 << 19,
        MRT = 1 << 20,  // lines
        MRT_TEXT = 1 << 21,
        MAN_MADE = 1 << 22,
        MAN_MADE_TEXT = 1 << 23,

        MRT_POINT = 1 << 24,           // points
        MRT_POINT_TEXT = 1 << 25,      // points
        FLIGHTS = 1 << 26,             // live airflight markers (SHPT_POLYGON)
        FLIGHTS_TEXT = 1 << 27,        // live airflight callsign labels
        FLIGHT_TRAILS = 1 << 28,       // live airflight trails (SHPT_ARC)
        FLIGHT_TRAILS_TEXT = 1 << 29,
        BUS_ROUTES = 1 << 30,          // bus route lines
        BUS_ROUTES2 = 1ULL << 31,         // bus route lines2
        BUS_ROUTES_TEXT = 1ULL << 32,  // bus route stop labels
        BUS_STOPS = 1ULL << 33,        // bus stop nodes
        BUS_STOPS_TEXT = 1ULL << 34,   // bus stop labels
        BUS_TRACKS = 1ULL << 35,       // bus tracking symbols
        BUS_TRACKS_TEXT = 1ULL << 36,  // bus tracking vehicle labels
        BUS_TRACKS_WINDSHIELD = 1ULL << 37,
        BUS_STOPS2 = 1ULL << 38  // return-direction bus stop nodes
    };

    unsigned int myLog2(uint64_t value)
    {
        unsigned int targetlevel = 0;
        while (value >>= 1)
        {
            ++targetlevel;
        }
        return targetlevel;
    }

    // template<int N> void printMrtStringToScreen();
    // template<> void printMrtStringToScreen<0>();

    // template<int N> void printStringToScreen(MapLayer & a_layer);
    // template<> void printStringToScreen<0>(MapLayer & a_layer);

    TSDWindow();
    ~TSDWindow();
    void initialize();
    void render();

    inline int getFps() { return m_fps; }

    void drawEBL(float x, float y, float r);
    void drawMRTStation();

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

    // Request route information for a bus service number.
    void fetchBusRoute(const QString& busNo, const QString& accountKey = QString());
    void onBusRouteReady(const QString& busNo, const QList<BusRoute>& routes);

    // Request live bus tracking for a bus stop number.
    void trackBusStop(const QString& stopCode, const QString& accountKey = QString());
    void onBusArrivalUpdated(const BusStopSnapshot& snapshot);

    // Clear all bus routes and tracks from map
    void clearBusInfo();

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
    std::array<QOpenGLShaderProgram**, 3> m_programSlots;
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

    std::uint64_t m_displayMask;

    BaseMapLayer m_sgCoastal;
    BaseMapLayer m_sgManMade;

    StaticMapLayer m_sgWaterArea;
    StaticMapLayer m_sgAmenities;
    StaticMapLayer m_sgPlaces;
    StaticMapLayer m_sgLandUsages;
    StaticMapLayer m_sgMRT;
    StaticMapLayer m_sgBuilding;
    StaticMapLayer m_sgMainRoads;
    StaticMapLayer m_sgMotorWays;
    StaticMapLayer m_sgMinorRoads;
    StaticMapLayer m_sgAirWays;
    // Live airflight layers. Rendered through the normal map-layer passes
    // (trails as SHPT_ARC lines, markers as SHPT_POLYGON silhouettes) instead
    // of the old QPainter overlay. Their geometry is rebuilt through the
    // common live-layer loop.
    LiveMapLayer m_sgFlightTrails;
    LiveMapLayer m_sgFlightMarkers;

    // Live bus route and vehicle tracking layers.
    LiveMapLayer m_sgBusRouteLines;
    LiveMapLayer m_sgBusRouteLines2;
    LiveMapLayer m_sgBusStops;
    LiveMapLayer m_sgBusStops2;
    LiveMapLayer m_sgBusVehicles;
    LiveMapLayer m_sgBusWindshields;

    QVector<BaseMapLayer*> m_baseLayers;
    QVector<StaticMapLayer*> m_staticLayers;
    QVector<LiveMapLayer*> m_liveLayers;
    QVector<LiveMapLayer*> m_busRouteLayers;
    QVector<LiveMapLayer*> m_busArrivalTimeLayers;
    
    GLfloat* m_mrt;
    bool m_bAutoZoom;
    bool m_bAutoSwing;
    bool m_bShaderToys;

    std::vector<std::unique_ptr<WorkerEntry>> m_workers;

    QHash<QString, TrackedAircraft> m_flightTable;

    // Bus Route worker thread, VBO, and current route data.
    QString m_accountKey;
    QList<BusRoute> m_activeBusRoutes;
    QString m_currentBusNo;

    // Bus Tracker snapshot, VBO, and vehicle infos.
    BusStopSnapshot m_currentBusStopSnapshot;
    void rebuildBusRouteLayers();
    void rebuildBusArrivalInfoLayers();
    void rebuildLiveLayers();
};

#endif  // TSDWINDOW_H
