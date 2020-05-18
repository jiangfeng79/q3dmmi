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

#include "shpReader.h"
#include "dbfReader.h"

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

	typedef struct _mapProperty
	{
		float scale;
		float centerX, centerY;
		float width, height;
		float mapBuildScale;
		int totalNumberOfVertex;
	} MapProperty;

	class MapLayer
	{
	public:
		MapLayer(const char* fileName) :m_vertex(NULL), m_color(NULL) {
			m_property.totalNumberOfVertex = 0;
			m_property.mapBuildScale = 111319.4907777778;
			m_property.scale = 0.1;
			m_fileName = QString(fileName);
		}

		MapLayer(const char* fileName, DisplayMaskBits id, DisplayMaskBits text_id) :
			m_vertex(NULL)
			, m_color(NULL)
			, m_id(id)
			, m_text_id(text_id)
			, m_bToFill(false)
		{
			m_property.totalNumberOfVertex = 0;
			m_property.mapBuildScale = 111319.4907777778;
			m_property.scale = 0.1;
			m_fileName = QString(fileName);
			m_layerName.clear();
		}

		MapLayer(const char* fileName, const char* layerName, DisplayMaskBits id, DisplayMaskBits text_id) :
			m_vertex(NULL)
			, m_color(NULL)
			, m_id(id)
			, m_text_id(text_id)
			, m_bToFill(false)
		{
			m_property.totalNumberOfVertex = 0;
			m_property.mapBuildScale = 111319.4907777778;
			m_property.scale = 0.1;
			m_fileName = QString(fileName);
			m_layerName = QString(layerName);
		}

		MapProperty m_property;
		QString m_fileName;
		QString m_layerName;
		std::vector <int> m_ring;
		std::vector <int> m_renderType;
		ShpReader m_shapeFileReader;
		DBFReader m_dbfFileReader;
		//DbfReader m_dbfFileReader
		GLuint m_VBO_ID[2];
		GLfloat* m_vertex;
		GLfloat* m_color;

		DisplayMaskBits m_id;
		DisplayMaskBits m_text_id;

		bool m_bToFill;

		void readData()
		{
			QString l_qsShpFileName = m_fileName + QString(".shp");
			QString l_qsDbfFileName = m_fileName + QString(".dbf");
			if (m_layerName.isEmpty())
			{
				m_dbfFileReader.read(l_qsDbfFileName.toStdString().c_str());
				m_shapeFileReader.read(l_qsShpFileName.toStdString().c_str());
			}
			else
			{
				m_dbfFileReader.readLayer(l_qsDbfFileName.toStdString().c_str(), m_layerName.toStdString().c_str());
				m_shapeFileReader.readLayer(l_qsShpFileName.toStdString().c_str(), m_dbfFileReader);
			}
		}
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
	void drawLayerAndFill(MapLayer& a_layer, int a_iColorId = 0);
	void drawLayer(MapLayer& a_layer, int a_iColorId = 0);
	void drawText(MapLayer& a_layer);
	void drawTextWithAngle(MapLayer& a_layer);
	void drawEBL(float x, float y, float r);
	void drawMRTStation();
	void centerMap();
	void setDisplayMask(DisplayMaskBits layer, bool b);
	inline void setAutoZoom(bool value) { m_bAutoZoom = value; }
	inline bool getAutoZoom() { return m_bAutoZoom; }

	inline void setAutoSwing(bool value) { m_bAutoSwing = value; }
	inline bool getAutoSwing() { return m_bAutoSwing; }

private:
	GLuint loadShader(GLenum type, const char* source);
	/*
	GLvoid buildFont(GLvoid);
	GLvoid glPrint(const char *fmt, ...);
	GLuint	m_fontBase;
	*/
	GLuint m_posAttr;
	GLuint m_colAttr;
	GLuint m_colorIdAttr;
	GLuint m_matrixUniform;
	GLuint m_mouse;
	GLuint m_resolution;
	GLuint m_time;
	GLuint m_colorId;

	GLuint m_listIndex;

	QOpenGLShaderProgram* m_program;

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
};

#endif // TSDWINDOW_H

