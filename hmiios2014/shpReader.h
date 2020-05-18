#ifndef SHPREAD_H_
#define SHPREAD_H_
#include <shapefil.h>
#include <stdlib.h>
#include "dbfReader.h"

//#define MAX_VERTEX 200000
class ShpReader
{
public:
	ShpReader(void);
	~ShpReader(void);

	void freeMemory();

	typedef struct _ShpEntity
	{
		unsigned int totalVertex;
		double minX, minY;
		double maxX, maxY;
		double** coordinate;//[MAX_VERTEX][3];
		unsigned char* isRing;//[MAX_VERTEX];
		unsigned int type;
	}ShpEntity;

	int read(const char* filename);
	int readLayer(const char* filename, DBFReader& layer);

	inline ShpEntity* getEntity() { return entity; };
	inline double getShpMinX() { return shpMinX; };
	inline double getShpMinY() { return shpMinY; };
	inline double getShpMaxX() { return shpMaxX; };
	inline double getShpMaxY() { return shpMaxY; };
	inline unsigned int getNumberOfEntity() { return numberOfEntity; };

protected:
	ShpEntity* entity;
	unsigned int numberOfEntity;
	double shpMinX, shpMinY;
	double shpMaxX, shpMaxY;
};
#endif /* SHPREAD_H_ */
