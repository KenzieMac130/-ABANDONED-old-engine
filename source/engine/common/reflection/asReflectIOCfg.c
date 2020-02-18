#include "asReflectIOCfg.h"

#include "asReflectImpliment.h"
#include "asReflectIOCfg.h"

#include "../asConfigFile.h"

#define INI_IMPLEMENTATION
#include "mattias/ini.h"

ASEXPORT asResults asReflectSaveToCfg(asCfgFile_t* pDest, const asReflectContainer* pReflectData, const unsigned char* pSrc)
{
	ini_t* pIni = asCfgGetMattiasPtr(pDest);
	if (!pIni)
	{
		return AS_FAILURE_INVALID_PARAM;
	}
	int currentSection = asCfgGetSectionIndex(pDest);

	const int memberCount = asReflectContainerMemberCount(pReflectData);
	for (int i = 0; i < memberCount; i++)
	{
		char resultBuffer[1024];
		memset(resultBuffer, 0, 1024);

		/*Type is C String*/
		if (strncmp(pReflectData->data[i].typeName, "char[]", 30) == 0)
		{
			strncpy(resultBuffer, &pSrc[pReflectData->data[i].dataOffset],
				pReflectData->data[i].dataSize > 1023 ? 1023 : pReflectData->data[i].dataSize);
		}
		/*Number*/
		else
		{
			int64_t astype = 0;
			bool isInt = false;
			double asdouble = 0.0;
			bool isFloat = false;

			/*Type is float*/
			if (strncmp(pReflectData->data[i].typeName, "float", 30) == 0)
			{
				asdouble = *(float*)&pSrc[pReflectData->data[i].dataOffset];
				isFloat = true;
			}
			/*Type is double*/
			else if (strncmp(pReflectData->data[i].typeName, "double", 30) == 0)
			{
				asdouble = *(double*)&pSrc[pReflectData->data[i].dataOffset];
				isFloat = true;
			}

			/*Type is int8*/
			else if (strncmp(pReflectData->data[i].typeName, "int8_t", 30) == 0 || strncmp(pReflectData->data[i].typeName, "char", 30) == 0)
			{
				astype = *(int8_t*)&pSrc[pReflectData->data[i].dataOffset];
				isInt = true;
			}
			/*Type is int16*/
			else if (strncmp(pReflectData->data[i].typeName, "int16_t", 30) == 0 || strncmp(pReflectData->data[i].typeName, "short", 30) == 0)
			{
				astype = *(int16_t*)&pSrc[pReflectData->data[i].dataOffset];
				isInt = true;
			}
			/*Type is int32*/
			else if (strncmp(pReflectData->data[i].typeName, "int32_t", 30) == 0 || strncmp(pReflectData->data[i].typeName, "int", 30) == 0)
			{
				astype = *(int32_t*)&pSrc[pReflectData->data[i].dataOffset];
				isInt = true;
			}
			/*Type is int64*/
			else if (strncmp(pReflectData->data[i].typeName, "int64_t", 30) == 0 || strncmp(pReflectData->data[i].typeName, "long long", 30) == 0)
			{
				astype = *(int64_t*)&pSrc[pReflectData->data[i].dataOffset];
				isInt = true;
			}

			/*Type is uint8*/
			else if (strncmp(pReflectData->data[i].typeName, "uint8_t", 30) == 0 || strncmp(pReflectData->data[i].typeName, "unsigned char", 30) == 0)
			{
				astype = *(uint8_t*)&pSrc[pReflectData->data[i].dataOffset];
				isInt = true;
			}
			/*Type is uint16*/
			else if (strncmp(pReflectData->data[i].typeName, "uint16_t", 30) == 0 || strncmp(pReflectData->data[i].typeName, "unsigned short", 30) == 0)
			{
				astype = *(uint16_t*)&pSrc[pReflectData->data[i].dataOffset];
				isInt = true;
			}
			/*Type is uint32*/
			else if (strncmp(pReflectData->data[i].typeName, "uint32_t", 30) == 0 || strncmp(pReflectData->data[i].typeName, "unsigned int", 30) == 0)
			{
				astype = *(uint32_t*)&pSrc[pReflectData->data[i].dataOffset];
				isInt = true;
			}
			/*Type is uint64*/
			else if (strncmp(pReflectData->data[i].typeName, "uint64_t", 30) == 0 || strncmp(pReflectData->data[i].typeName, "unsigned long long", 30) == 0)
			{
				astype = (int64_t) * (uint64_t*)&pSrc[pReflectData->data[i].dataOffset];
				isInt = true;
			}

			/*Format as integer*/
			if (isInt)
			{
				snprintf(resultBuffer, 1023, "%lld", astype);
			}
			else if (isFloat)
			{
				snprintf(resultBuffer, 1023, "%lf", asdouble);
			}
			/*Unsupported type*/
			else
			{
				continue;
			}
		}

		/*Add Property*/
		resultBuffer[1023] = '\0';
		ini_property_add(pIni, currentSection, pReflectData->data[i].varName, strnlen(pReflectData->data[i].varName, 30), resultBuffer, strnlen(resultBuffer, 1024));
	}

	return AS_SUCCESS;
}

ASEXPORT asResults asReflectLoadFromCfg(unsigned char* pDest, const size_t destSize, const asReflectContainer* pDestReflectData, asCfgFile_t* pSrc)
{
	if (!pSrc)
	{
		return AS_FAILURE_INVALID_PARAM;
	}
	
	/*Todo:*/
	return AS_FAILURE_UNKNOWN;
}
