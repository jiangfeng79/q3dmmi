#include "shpReader.h"

ShpReader::ShpReader(void) : entity(NULL), numberOfEntity(0), shpMinX(0), shpMaxX(0), shpMinY(0), shpMaxY(0) {}


ShpReader::~ShpReader(void) {
    std::lock_guard<std::recursive_mutex> lk(m_mutex);
    // free
    if(entity != NULL && numberOfEntity >0)
    {
        for (unsigned int i = 0; i < numberOfEntity; ++i)
        {
            if (entity[i].coordinate != NULL)
            {
                for (unsigned int j = 0; j < entity[i].totalVertex; ++j)
                {
                    free(entity[i].coordinate[j]);
                }
                free(entity[i].coordinate);
            }
            free(entity[i].isRing);
        }
        free(entity);
    }
}

void ShpReader::freeMemory() {
    std::lock_guard<std::recursive_mutex> lk(m_mutex);
    if (entity != NULL && numberOfEntity > 0) {
        for (unsigned int i = 0; i < numberOfEntity; ++i)
        {
            if (entity[i].coordinate != NULL)
            {
                for (unsigned int j = 0; j < entity[i].totalVertex; ++j)
                {
                    free(entity[i].coordinate[j]);
                }
                free(entity[i].coordinate);
            }
            free(entity[i].isRing);
        }
        free(entity);
        entity = NULL;
    }
}

int ShpReader::read(const char* filename) {
    std::lock_guard<std::recursive_mutex> lk(m_mutex);
    SHPHandle hSHP;
    int nShapeType, nEntities, i, iPart;
    double adfMinBound[4], adfMaxBound[4];

    hSHP = SHPOpen(filename, "rb");

    if (hSHP == NULL)
    {
        printf("Unable to open:%s\n", filename);
        exit(1);
    }

    SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

    // jiangfeng: create the memory
    entity = (ShpEntity*)malloc(sizeof(ShpEntity) * nEntities);
    shpMinX = adfMinBound[0];
    shpMinY = adfMinBound[1];
    shpMaxX = adfMaxBound[0];
    shpMaxY = adfMaxBound[1];

    numberOfEntity = nEntities;

    if (entity)
    {
        for (i = 0; i < nEntities; i++)
        {
            int j;
            SHPObject* psShape;

            psShape = SHPReadObject(hSHP, i);

            // jiangfeng: min, max
            entity[i].minX = psShape->dfXMin;
            entity[i].minY = psShape->dfYMin;

            entity[i].maxX = psShape->dfXMax;
            entity[i].maxY = psShape->dfYMax;

            entity[i].type = psShape->nSHPType;
            // jiangfeng: now allocate the memory space for vertexes
            entity[i].coordinate = (double**)malloc(sizeof(double) * (psShape->nVertices));
            for (j = 0; j < psShape->nVertices; j++)
            {
                entity[i].coordinate[j] = (double*)malloc(3 * sizeof(double));
            }

            entity[i].isRing = (unsigned char*)malloc(sizeof(unsigned char) * (psShape->nVertices));

            if (psShape->nParts > 0 && psShape->panPartStart[0] != 0)
            {
                printf("panPartStart[0] = %d, not zero as expected.\n", psShape->panPartStart[0]);
            }

            entity[i].totalVertex = psShape->nVertices;
            for (j = 0, iPart = 1; j < psShape->nVertices; j++)
            {
                if (iPart < psShape->nParts && psShape->panPartStart[iPart] == j)
                {
                    iPart++;
                    entity[i].isRing[j] = 1;
                }
                else
                {
                    entity[i].isRing[j] = 0;
                }

                entity[i].coordinate[j][0] = psShape->padfX[j];
                entity[i].coordinate[j][1] = psShape->padfY[j];
                entity[i].coordinate[j][2] = psShape->padfZ[j];
            }

            SHPDestroyObject(psShape);
        }

        SHPClose(hSHP);
    }
    return 0;
}

int ShpReader::readLayer(const char* filename, DBFReader& layer) {
    std::lock_guard<std::recursive_mutex> lk(m_mutex);
    SHPHandle hSHP;
    int nShapeType, nEntities, i, iPart;
    double adfMinBound[4], adfMaxBound[4];

    hSHP = SHPOpen(filename, "rb");

    if (hSHP == NULL)
    {
        exit(1);
    }

    SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

    nEntities = layer.getNumberOfRecords();

    // jiangfeng: create the memory
    entity = (ShpEntity*)malloc(sizeof(ShpEntity) * nEntities);
    shpMinX = adfMinBound[0];
    shpMinY = adfMinBound[1];
    shpMaxX = adfMaxBound[0];
    shpMaxY = adfMaxBound[1];

    numberOfEntity = nEntities;

    if (entity)
    {
        for (i = 0; i < nEntities; i++)
        {
            int j;
            SHPObject* psShape;

            psShape = SHPReadObject(hSHP, (layer.getEntity())[i].id);

            // jiangfeng: min, max
            entity[i].minX = psShape->dfXMin;
            entity[i].minY = psShape->dfYMin;

            entity[i].maxX = psShape->dfXMax;
            entity[i].maxY = psShape->dfYMax;

            entity[i].type = psShape->nSHPType;
            // jiangfeng: now allocate the memory space for vertexes
            entity[i].coordinate = (double**)malloc(sizeof(double) * (psShape->nVertices));
            for (j = 0; j < psShape->nVertices; j++)
            {
                entity[i].coordinate[j] = (double*)malloc(3 * sizeof(double));
            }

            entity[i].isRing = (unsigned char*)malloc(sizeof(unsigned char) * (psShape->nVertices));

            if (psShape->nParts > 0 && psShape->panPartStart[0] != 0)
            {
                printf("panPartStart[0] = %d, not zero as expected.\n", psShape->panPartStart[0]);
            }

            entity[i].totalVertex = psShape->nVertices;
            for (j = 0, iPart = 1; j < psShape->nVertices; j++)
            {
                if (iPart < psShape->nParts && psShape->panPartStart[iPart] == j)
                {
                    iPart++;
                    entity[i].isRing[j] = 1;
                }
                else
                {
                    entity[i].isRing[j] = 0;
                }

                entity[i].coordinate[j][0] = psShape->padfX[j];
                entity[i].coordinate[j][1] = psShape->padfY[j];
                entity[i].coordinate[j][2] = psShape->padfZ[j];
            }

            SHPDestroyObject(psShape);
        }

        SHPClose(hSHP);
    }
    return 0;
}
