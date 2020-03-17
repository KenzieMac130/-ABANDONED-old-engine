#include "MacroParser.h"

asResults processMacros(FxCreator* pCreator, const char* content, size_t contentSize, const char* fileName)
{
	return AS_SUCCESS;
	if (re_match("#pragma asIgnoreMacroParser", content) >= 0) { return AS_SUCCESS; }
	int dataSectionDepth = 0;

	char* next = content;
	while(1) {
		int foundPos = re_match("AS_REFLECT_", next);
		if (foundPos <= 0) { break; }
		foundPos += 11;
		int openBracket = re_match("(", &next[foundPos]);
		if (openBracket <= 0) { return AS_FAILURE_INVALID_PARAM; }
		int closeBracket = re_match(")", &next[foundPos + openBracket]);
		if (closeBracket <= 0) { return AS_FAILURE_INVALID_PARAM; }

		char command[32];
		memset(command, 0, 32);
		strncat(command, &next[foundPos], openBracket > 31 ? 31 : openBracket);

		char paramsBody[2048];
		memset(paramsBody, 0, 2048);
		strncat(paramsBody, &next[foundPos + openBracket + 1], closeBracket-1 > 2047 ? 2047 : closeBracket-1);
		
		/*Reflect Macros*/
		if (asIsSizedStringEqual(command, "BEGIN", 32))
		{
			//FxCreator_StartDataSection(pCreator, paramsBody);
		}
		else if (asIsSizedStringEqual(command, "ENTRY", 32))
		{
			char typeName[30];
			char varName[30];
			memset(typeName, 0, 30);
			memset(varName, 0, 30);

			/*Todo:*/
			//FxCreator_DataSection_AddEntry(pCreator, typeName, varName, NULL, 0);
		}
		else if (asIsSizedStringEqual(command, "EXTERN_C_STRING", 32))
		{
			/*Todo:*/
		}
		else if (asIsSizedStringEqual(command, "EXTERN_RESOURCE_FILE_ID", 32))
		{
			/*Todo:*/
		}
		else if (asIsSizedStringEqual(command, "END", 32))
		{
			//FxCreator_EndDataSection(pCreator);
		}

		/*Progress*/
		int nextReturn = re_match("\n", next);
		if (nextReturn > 0) { break; }
		next += nextReturn;
	}

	if (dataSectionDepth != 0)
	{
		asDebugLog("[ERROR]> Serialization Sections Improperly Terminated in %s", fileName);
		return AS_FAILURE_INVALID_PARAM;
	}

	return AS_SUCCESS;
}
