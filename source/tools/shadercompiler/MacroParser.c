#include "MacroParser.h"

size_t _findCmdStart(char* content, size_t lastEnd, size_t contentSize)
{
	size_t start = lastEnd;
	if (lastEnd + 1 < contentSize)
	{
		start = lastEnd + 1;
	}
	return start;
}

size_t _findCmdEnd(char* content, size_t start, size_t contentSize)
{
	for (size_t i = start; i < contentSize; i++)
	{
		if (content[i] == ';')
		{
			return i;
		}
	}
	return start;
}

#define CMD_MAX 4096
asResults processMacros(FxCreator* pCreator, const char* content, size_t contentSize)
{
	size_t cmdStart = 0;
	size_t cmdEnd = _findCmdEnd(content, 0, contentSize);

	bool sectionStart = false;
	while (cmdStart != cmdEnd)
	{
		char command[CMD_MAX];
		memset(command, 0, CMD_MAX);
		strncpy(command, &content[cmdStart], cmdEnd - cmdStart > CMD_MAX-1 ? CMD_MAX-1 : cmdEnd - cmdStart);
		command[cmdEnd] = '\0';
		size_t cmdArgsBegin = 0;
		for (size_t i = 0; i < CMD_MAX - 1; i++) { if (command[i] == '(') { command[i] = '\0'; cmdArgsBegin = i + 1; break; } }
		for (size_t i = 0; i < CMD_MAX - 1; i++) { if (command[i] == ')') { command[i] = '\0'; } }
		/*Fixup Passing " by Substituting useless @, because passing args to an app with spaces and " is painful*/
		for (size_t i = 0; i < CMD_MAX - 1; i++) { if (command[i] == '@') { command[i] = '"'; } }
		char* commandArgs = &command[cmdArgsBegin];
		asDebugLog("Reflect Data: %s(%s)", command, commandArgs);

		/*Reflect Macros*/
		if (asIsSizedStringEqual(command, "BEGIN", CMD_MAX))
		{
			if (sectionStart) { return AS_FAILURE_INVALID_PARAM; }
			if (FxCreator_StartDataSection(pCreator, commandArgs) != AS_SUCCESS) { return AS_FAILURE_DUPLICATE_ENTRY; }
			sectionStart = true;
		}
		else if (asIsSizedStringEqual(command, "ENTRY", CMD_MAX))
		{

			if (!sectionStart) { return AS_FAILURE_INVALID_PARAM; }
			char* typeName = strtok(commandArgs, ", ");
			char* varName = strtok(NULL, ", ");
			char* valueStr = strtok(NULL, "\"");

			size_t floatCount = 0;
			float floats[32] = { 0 };

			size_t valueStrLen = strlen(valueStr);
			char* pNext = valueStr;
			while (pNext != NULL && pNext < valueStr + valueStrLen)
			{
				char* currPtr = pNext;
				floats[floatCount] = strtof(currPtr, &pNext);
				if(currPtr == pNext)
				{
					pNext++;
				}
				else
				{
					floatCount++;
				}
			}
			if (FxCreator_DataSection_AddEntry(pCreator, typeName, varName, floats, floatCount) != AS_SUCCESS) { return AS_FAILURE_OUT_OF_MEMORY; }
		}
		else if (asIsSizedStringEqual(command, "EXTERN_C_STRING", CMD_MAX))
		{
			if (!sectionStart) { return AS_FAILURE_INVALID_PARAM; }
			char* varName = strtok(commandArgs, ", ");
			char* valueStr = strtok(NULL, "");
			if (FxCreator_DataSection_AddEntry(pCreator, "char[]", varName, valueStr, strlen(valueStr)+1) != AS_SUCCESS) { return AS_FAILURE_OUT_OF_MEMORY; }
		}
		else if (asIsSizedStringEqual(command, "EXTERN_RESOURCE_FILE_ID", CMD_MAX))
		{
			if (!sectionStart) { return AS_FAILURE_INVALID_PARAM; }
			char* varName = strtok(commandArgs, ", ");
			char* valueStr = strtok(NULL, "");
			asResourceFileID_t fileId = asResource_FileIDFromRelativePath(valueStr, strlen(valueStr));
			if (FxCreator_DataSection_AddEntry(pCreator, "asResourceFileID_t", varName, &fileId, sizeof(asResourceFileID_t)) != AS_SUCCESS) { return AS_FAILURE_OUT_OF_MEMORY; }
		}
		else if (asIsSizedStringEqual(command, "END", CMD_MAX))
		{
			if (!sectionStart) { return AS_FAILURE_INVALID_PARAM; }
			FxCreator_EndDataSection(pCreator);
			sectionStart = false;
		}

		cmdStart = _findCmdStart(content, cmdEnd, contentSize);
		cmdEnd = _findCmdEnd(content, cmdStart, contentSize);
	}

	if (sectionStart) { return AS_FAILURE_INVALID_PARAM; }
	return AS_SUCCESS;
}
