#include "dbfReader.h"
#include <qDebug>


DBFReader::DBFReader(void) :entity(NULL)
{
}

DBFReader::~DBFReader(void)
{
	//free memory
	if (entity != NULL && numberOfEntity > 0)
	{
		for (unsigned int i = 0; i < numberOfEntity; ++i)
		{
			if (entity[i].type == FTString)
			{
				if (entity[i].stringValue)
					free(entity[i].stringValue);
			}
		}
		free(entity);
		numberOfEntity = 0;
		entity = NULL;
	}
}

void DBFReader::freeMemory()
{
	if (entity != NULL && numberOfEntity > 0)
	{

		for (unsigned int i = 0; i < numberOfEntity; ++i)
		{
			if (entity[i].type == FTString)
			{
				if (entity[i].stringValue)
					free(entity[i].stringValue);
			}
		}

		free(entity);
		numberOfEntity = 0;
		entity = NULL;
	}
}

int DBFReader::read(const char * filename)
{
	DBFHandle	hDBF;
	int		i, iRecord;
	char		szFormat[32];
	int		nWidth, nDecimals;
	char	szTitle[12];

	/* -------------------------------------------------------------------- */
	/*      Open the file.                                                  */
	/* -------------------------------------------------------------------- */
	hDBF = DBFOpen(filename, "rb");
	if (hDBF == NULL)
	{
		printf("DBFOpen(%s,\"r\") failed.\n", filename);
		return(2);
	}

	/* -------------------------------------------------------------------- */
	/*	If there is no data in this file let the user know.		*/
	/* -------------------------------------------------------------------- */
	if (DBFGetFieldCount(hDBF) == 0)
	{
		printf("There are no fields in this table!\n");
		return(3);
	}

	//jiangfeng: create the memory
	numberOfEntity = DBFGetRecordCount(hDBF);
	entity = (DBFEntity *)malloc(sizeof(DBFEntity)*numberOfEntity);

	if (entity)
	{
		for (iRecord = 0; iRecord < DBFGetRecordCount(hDBF); iRecord++)
		{
			entity[iRecord].type = FTString;
			entity[iRecord].stringValue = NULL;
			for (i = 0; i < DBFGetFieldCount(hDBF); i++)
			{
				DBFFieldType	eType;
				eType = DBFGetFieldInfo(hDBF, i, szTitle, &nWidth, &nDecimals);
				if (!strcmp(szTitle, "name"))
				{
					if (!DBFIsAttributeNULL(hDBF, iRecord, i))
						//entity[iRecord].stringValue = NULL; //null ptr, int 0 and double????
					//else
					{
						switch (eType)
						{
						case FTString:
						{
							entity[iRecord].stringValue = (char *)malloc(strlen(DBFReadStringAttribute(hDBF, iRecord, i)) + 1);
							sprintf(entity[iRecord].stringValue, DBFReadStringAttribute(hDBF, iRecord, i));
						}
						break;
						/*
						case FTInteger:
						//entity[iRecord].intValue = DBFReadIntegerAttribute( hDBF, iRecord, i ) ;
						break;

					case FTDouble:
						//entity[iRecord].doubleValue = DBFReadDoubleAttribute( hDBF, iRecord, i );
						break;
						*/
						default:
							break;
						}
					}
				}//else
					//entity[iRecord].stringValue = NULL;
			}
		}
	}
	DBFClose(hDBF);
	return(0);
}

int DBFReader::readLayer(const char * filename, const char * layername)
{
	DBFHandle	hDBF;
	int		i, iRecord;
	char		szFormat[32];
	int		nWidth, nDecimals;
	char	szTitle[12];

	/* -------------------------------------------------------------------- */
	/*      Open the file.                                                  */
	/* -------------------------------------------------------------------- */
	hDBF = DBFOpen(filename, "rb");
	if (hDBF == NULL)
	{
		printf("DBFOpen(%s,\"r\") failed.\n", filename);
		return(2);
	}

	/* -------------------------------------------------------------------- */
	/*	If there is no data in this file let the user know.		*/
	/* -------------------------------------------------------------------- */
	if (DBFGetFieldCount(hDBF) == 0)
	{
		printf("There are no fields in this table!\n");
		return(3);
	}

	//jiangfeng: create the memory
	numberOfEntity = DBFGetRecordCount(hDBF);
	entity = (DBFEntity *)malloc(sizeof(DBFEntity)*numberOfEntity);

	int l_iNumberOfLayerItem = 0;
	if (entity)
	{
		for (iRecord = 0; iRecord < DBFGetRecordCount(hDBF); iRecord++)
		{
			//entity[iRecord].type = FTString;
			//entity[iRecord].stringValue = NULL;
			for (i = 0; i < DBFGetFieldCount(hDBF); i++)
			{
				DBFFieldType	eType;
				eType = DBFGetFieldInfo(hDBF, i, szTitle, &nWidth, &nDecimals);
				if (!strcmp(szTitle, layername) /*|| !strcmp(szTitle,"amenity")*/)
				{
					if (!DBFIsAttributeNULL(hDBF, iRecord, i))
						//entity[iRecord].stringValue = NULL; //null ptr, int 0 and double????
					//else
					{
						switch (eType)
						{
						case FTString:
						{
							//if(!strcmp(szTitle,"name"))
							//{
							entity[l_iNumberOfLayerItem].id = iRecord;
							entity[l_iNumberOfLayerItem].stringValue = (char *)malloc(strlen(DBFReadStringAttribute(hDBF, iRecord, i)) + 1);
							sprintf(entity[l_iNumberOfLayerItem].stringValue, DBFReadStringAttribute(hDBF, iRecord, i));
							//}
						}
						break;
						/*
						case FTInteger:
						//entity[iRecord].intValue = DBFReadIntegerAttribute( hDBF, iRecord, i ) ;
						break;

					case FTDouble:
						//entity[iRecord].doubleValue = DBFReadDoubleAttribute( hDBF, iRecord, i );
						break;
						*/
						default:
							break;
						}
						l_iNumberOfLayerItem++;
					}
				}//else
					//entity[iRecord].stringValue = NULL;
			}
		}
		numberOfEntity = l_iNumberOfLayerItem;
	}

	DBFClose(hDBF);
	return(0);
}


