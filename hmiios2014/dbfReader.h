#ifndef DBFREAD_H_
#define DBFREAD_H_
#include <shapefil.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//#define MAX_VERTEX 200000
class DBFReader
{
public:
	DBFReader(void);
	~DBFReader(void);

	void freeMemory();// { if(entity) free(entity); entity = NULL;}

	typedef struct _DBFEntity
	{
		int id;
		double coordinate[2];//[MAX_VERTEX][3];
		double angle;
		DBFFieldType type;
		union {
			int intValue;
			double doubleValue;
			char* stringValue;
		};
	}DBFEntity;

	int read(const char* filename);
	int readLayer(const char* filename, const char* layername);

	inline DBFEntity* getEntity() { return entity; };
	inline unsigned int getNumberOfRecords() { return numberOfEntity; }

protected:
	DBFEntity* entity;
	unsigned int numberOfEntity;
};
#endif /* DBFREAD_H_ */