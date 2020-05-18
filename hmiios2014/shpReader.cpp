#include "shpReader.h"

ShpReader::ShpReader(void) :entity(NULL), numberOfEntity(0), shpMinX(0), shpMaxX(0), shpMinY(0), shpMaxY(0)
{
}


ShpReader::~ShpReader(void)
{
	//free memory
	if (entity != NULL && numberOfEntity > 0)
	{
		for (unsigned int i = 0; i < numberOfEntity; ++i)
		{
			free(entity[i].coordinate);
			free(entity[i].isRing);
		}
		free(entity);
	}
}

void ShpReader::freeMemory()
{
	//free memory
	if (entity != NULL && numberOfEntity > 0)
	{
		for (unsigned int i = 0; i < numberOfEntity; ++i)
		{
			free(entity[i].coordinate);
			free(entity[i].isRing);
		}
		free(entity);
		entity = NULL;
	}
}

int ShpReader::read(const char* filename)
{
	SHPHandle	hSHP;
	int			nShapeType, nEntities, i, iPart, bValidate = 0, nInvalidCount = 0;
	int			bHeaderOnly = 0;
	const char* pszPlus;
	double 		adfMinBound[4], adfMaxBound[4];

	/* -------------------------------------------------------------------- */
	/*      Open the passed shapefile.                                      */
	/* -------------------------------------------------------------------- */
	hSHP = SHPOpen(filename, "rb");

	if (hSHP == NULL)
	{
		printf("Unable to open:%s\n", filename);
		exit(1);
	}
	else
		printf("Opened shp file: %s\n", filename);

	/* -------------------------------------------------------------------- */
	/*      Print out the file bounds.                                      */
	/* -------------------------------------------------------------------- */
	SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

	printf("Shapefile Type: %s   # of Shapes: %d\n\n", SHPTypeName(nShapeType), nEntities);
	printf("File Bounds: (%f,%f,%f,%f) to  (%f,%f,%f,%f)\n\n", adfMinBound[0], adfMinBound[1], adfMinBound[2], adfMinBound[3], adfMaxBound[0], adfMaxBound[1], adfMaxBound[2], adfMaxBound[3]);

	//jiangfeng: create the memory
	entity = (ShpEntity*)malloc(sizeof(ShpEntity) * nEntities);
	shpMinX = adfMinBound[0];
	shpMinY = adfMinBound[1];
	shpMaxX = adfMaxBound[0];
	shpMaxY = adfMaxBound[1];

	numberOfEntity = nEntities;

	if (entity)
	{
		/* -------------------------------------------------------------------- */
		/*	Skim over the list of shapes, printing all the vertices.	*/
		/* -------------------------------------------------------------------- */
		for (i = 0; i < nEntities && !bHeaderOnly; i++)
		{
			int		j;
			SHPObject* psShape;

			psShape = SHPReadObject(hSHP, i);

			//jiangfeng: min, max
			entity[i].minX = psShape->dfXMin;
			entity[i].minY = psShape->dfYMin;

			entity[i].maxX = psShape->dfXMax;
			entity[i].maxY = psShape->dfYMax;

			entity[i].type = psShape->nSHPType;
			//jiangfeng: now allocate the memory space for vertexes
			entity[i].coordinate = (double**)malloc(sizeof(double) * (psShape->nVertices));
			for (j = 0; j < psShape->nVertices; j++)
				entity[i].coordinate[j] = (double*)malloc(3 * sizeof(double));

			entity[i].isRing = (unsigned char*)malloc(sizeof(unsigned char) * (psShape->nVertices));

			if (psShape->nParts > 0 && psShape->panPartStart[0] != 0)
				printf("panPartStart[0] = %d, not zero as expected.\n", psShape->panPartStart[0]);

			entity[i].totalVertex = psShape->nVertices;
			for (j = 0, iPart = 1; j < psShape->nVertices; j++)
			{
				const char* pszPartType = "";

				if (j == 0 && psShape->nParts > 0)
					pszPartType = SHPPartTypeName(psShape->panPartType[0]);

				if (iPart < psShape->nParts
					&& psShape->panPartStart[iPart] == j)
				{
					pszPartType = SHPPartTypeName(psShape->panPartType[iPart]);
					iPart++;
					pszPlus = "+";
					entity[i].isRing[j] = 1;
				}
				else
				{
					pszPlus = " ";
					entity[i].isRing[j] = 0;
				}

				entity[i].coordinate[j][0] = psShape->padfX[j];
				entity[i].coordinate[j][1] = psShape->padfY[j];
				entity[i].coordinate[j][2] = psShape->padfZ[j];
			}

			if (bValidate)
			{
				int nAltered = SHPRewindObject(hSHP, psShape);

				if (nAltered > 0)
					nInvalidCount++;
			}
			SHPDestroyObject(psShape);
		}

		SHPClose(hSHP);

		if (bValidate)
			printf("%d object has invalid ring orderings.\n", nInvalidCount);
	}
#ifdef USE_DBMALLOC
	malloc_dump(2);
#endif
	return 0;
}

int ShpReader::readLayer(const char* filename, DBFReader& layer)
{
	SHPHandle	hSHP;
	int			nShapeType, nEntities, i, iPart, bValidate = 0, nInvalidCount = 0;
	int			bHeaderOnly = 0;
	const char* pszPlus;
	double 		adfMinBound[4], adfMaxBound[4];

	/* -------------------------------------------------------------------- */
	/*      Open the passed shapefile.                                      */
	/* -------------------------------------------------------------------- */
	hSHP = SHPOpen(filename, "rb");

	if (hSHP == NULL)
	{
		printf("Unable to open:%s\n", filename);
		exit(1);
	}
	else
		printf("Opened shp file: %s\n", filename);

	/* -------------------------------------------------------------------- */
	/*      Print out the file bounds.                                      */
	/* -------------------------------------------------------------------- */
	SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

	printf("Shapefile Type: %s   # of Shapes: %d\n\n", SHPTypeName(nShapeType), nEntities);
	printf("File Bounds: (%f,%f,%f,%f) to  (%f,%f,%f,%f)\n\n", adfMinBound[0], adfMinBound[1], adfMinBound[2], adfMinBound[3], adfMaxBound[0], adfMaxBound[1], adfMaxBound[2], adfMaxBound[3]);

	nEntities = layer.getNumberOfRecords();

	//jiangfeng: create the memory
	entity = (ShpEntity*)malloc(sizeof(ShpEntity) * nEntities);
	shpMinX = adfMinBound[0];
	shpMinY = adfMinBound[1];
	shpMaxX = adfMaxBound[0];
	shpMaxY = adfMaxBound[1];

	numberOfEntity = nEntities;

	if (entity)
	{
		/* -------------------------------------------------------------------- */
		/*	Skim over the list of shapes, printing all the vertices.	*/
		/* -------------------------------------------------------------------- */
		for (i = 0; i < nEntities && !bHeaderOnly; i++)
		{
			int		j;
			SHPObject* psShape;

			psShape = SHPReadObject(hSHP, (layer.getEntity())[i].id);

			//jiangfeng: min, max
			entity[i].minX = psShape->dfXMin;
			entity[i].minY = psShape->dfYMin;

			entity[i].maxX = psShape->dfXMax;
			entity[i].maxY = psShape->dfYMax;

			entity[i].type = psShape->nSHPType;
			//jiangfeng: now allocate the memory space for vertexes
			entity[i].coordinate = (double**)malloc(sizeof(double) * (psShape->nVertices));
			for (j = 0; j < psShape->nVertices; j++)
				entity[i].coordinate[j] = (double*)malloc(3 * sizeof(double));

			entity[i].isRing = (unsigned char*)malloc(sizeof(unsigned char) * (psShape->nVertices));

			if (psShape->nParts > 0 && psShape->panPartStart[0] != 0)
				printf("panPartStart[0] = %d, not zero as expected.\n", psShape->panPartStart[0]);

			entity[i].totalVertex = psShape->nVertices;
			for (j = 0, iPart = 1; j < psShape->nVertices; j++)
			{
				const char* pszPartType = "";

				if (j == 0 && psShape->nParts > 0)
					pszPartType = SHPPartTypeName(psShape->panPartType[0]);

				if (iPart < psShape->nParts
					&& psShape->panPartStart[iPart] == j)
				{
					pszPartType = SHPPartTypeName(psShape->panPartType[iPart]);
					iPart++;
					pszPlus = "+";
					entity[i].isRing[j] = 1;
				}
				else
				{
					pszPlus = " ";
					entity[i].isRing[j] = 0;
				}

				entity[i].coordinate[j][0] = psShape->padfX[j];
				entity[i].coordinate[j][1] = psShape->padfY[j];
				entity[i].coordinate[j][2] = psShape->padfZ[j];
			}

			if (bValidate)
			{
				int nAltered = SHPRewindObject(hSHP, psShape);

				if (nAltered > 0)
					nInvalidCount++;
			}
			SHPDestroyObject(psShape);
		}

		SHPClose(hSHP);

		if (bValidate)
			printf("%d object has invalid ring orderings.\n", nInvalidCount);
	}
#ifdef USE_DBMALLOC
	malloc_dump(2);
#endif
	return 0;
}
