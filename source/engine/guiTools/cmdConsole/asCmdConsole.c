#include "../../cimgui/asDearImGuiImplimentation.h"
#include "asCmdConsole.h"

#include "engine/common/asCommon.h"

size_t namespaceCount = 0;
#define MAX_NAMESPACES 64
#define NAMESPACE_MAX_LENGTH 32
asPreferenceManager* pNamespacePrefs[MAX_NAMESPACES];
char namespaceNames[MAX_NAMESPACES][NAMESPACE_MAX_LENGTH];

#define MAX_LINE_INPUT 4096
#define COMMAND_HISTORY 32

int32_t commandHistoryNewest = 0;
int32_t commandHistoryPoint = 0;
char commandHistory[COMMAND_HISTORY][MAX_LINE_INPUT] = { "COMMAND" };

int _commandEntryCallback(ImGuiInputTextCallbackData* input)
{
	bool changeHistory = false;
	if (input->EventKey == ImGuiKey_UpArrow)
	{
		if (commandHistoryPoint >= commandHistoryNewest)
		{
			memcpy(commandHistory[commandHistoryPoint % COMMAND_HISTORY], input->Buf, MAX_LINE_INPUT);
			commandHistoryPoint = commandHistoryNewest;
		}
		if (commandHistoryPoint > (commandHistoryNewest - COMMAND_HISTORY > 0 ? commandHistoryNewest - COMMAND_HISTORY : 0))
		{
			commandHistoryPoint--;
			changeHistory = true;
		}
	}
	if (input->EventKey == ImGuiKey_DownArrow)
	{
		if (commandHistoryPoint < commandHistoryNewest)
		{
			commandHistoryPoint++;
			changeHistory = true;
		}
	}
	if (changeHistory)
	{
		ImGuiInputTextCallbackData_DeleteChars(input, 0, input->BufTextLen);
		ImGuiInputTextCallbackData_InsertChars(input, 0, commandHistory[commandHistoryPoint % COMMAND_HISTORY], NULL);
	}
	return 0;
}

ASEXPORT asResults asGuiToolCommandConsole_RegisterPrefManager(asPreferenceManager* manager, const char* nameSpace)
{
	strncat(namespaceNames[namespaceCount], nameSpace, NAMESPACE_MAX_LENGTH - 1);
	pNamespacePrefs[namespaceCount] = manager;
	namespaceCount++;
	return AS_SUCCESS;
}

void displayHelp()
{
	asDebugLog(
		"----------Console Help----------\n"
		"Right-click box for command catalog\n"
		"(command)!: Display the current command value if applicaple\n"
		"(command)?: Display the current command help string\n"
		"-help: Show help for the console");
	for (size_t n = 0; n < namespaceCount; n++) /*Namespaces*/
	{
		size_t sectionCount = asPreferencesInspectGetSectionCount(pNamespacePrefs[n]);
		for (size_t s = 0; s < sectionCount; s++)
		{
			const char* sectionName = NULL;
			asPreferencesInspectGetSectionNameTmp(pNamespacePrefs[n], s, &sectionName);
			size_t entryCount = asPreferencesInspectGetEntryCount(pNamespacePrefs[n], s);
			for (size_t e = 0; e < entryCount; e++)
			{
				const char* entryName = NULL;
				asPreferencesInspectGetEntryNameTmp(pNamespacePrefs[n], s, e, &entryName);
				const char* helpString = asPreferencesGetParamHelp(pNamespacePrefs[n], sectionName, entryName);
				if (helpString) {
					asDebugLog("-%s.%s.%s: %s",
						namespaceNames[n],
						sectionName,
						entryName,
						helpString);
				}
				else {
					asDebugLog("-%s.%s.%s",
						namespaceNames[n],
						sectionName,
						entryName);
				}
			}
		}
	}
}

asResults enterCommand(const char* commandString, bool silent)
{
	if (!silent) { asDebugLog("Command-> %.*s", MAX_LINE_INPUT, commandString); }

	/*Help*/
	if (asIsStringEqual(commandString, "help"))
	{
		displayHelp();
		return AS_SUCCESS;
	}

	char buffer[MAX_LINE_INPUT];
	memset(buffer, 0, MAX_LINE_INPUT);
	strncat(buffer, commandString, MAX_LINE_INPUT);

	/*Digest Input*/
	char* parsePos = buffer;
	char* namespaceStr = parsePos;
	char* sectionStr = NULL;
	char* entryStr = NULL;
	char* arguementStr = NULL;
	bool startedWhitespace = false;
	bool helpRequested = false;
	bool valueRequested = false;

	while (*parsePos != '\0')
	{
		if (!entryStr && *parsePos == '.') /*Search for Section and Entry*/
		{
			*parsePos = '\0';
			if (!sectionStr) /*Search Section*/
			{
				if (*(parsePos + 1) != '\0') { sectionStr = parsePos + 1; }
			}
			else /*Search Entry*/
			{
				if (*(parsePos + 1) != '\0') { entryStr = parsePos + 1; }
			}
		}
		else if (entryStr && !startedWhitespace && (*parsePos == ' ')) /*Look for arguement whitespace*/
		{
			*parsePos = '\0';
			startedWhitespace = true;
		}
		else if (entryStr && !startedWhitespace && (*parsePos == '?')) /*Look for arguement as help*/
		{
			*parsePos = '\0';
			helpRequested = true;
		}
		else if (entryStr && !startedWhitespace && (*parsePos == '!')) /*Look for arguement as value*/
		{
			*parsePos = '\0';
			valueRequested = true;
		}
		else if (!arguementStr && startedWhitespace &! (*parsePos == ' ')) /*Look for beginning of arguement*/
		{
			arguementStr = parsePos;
		}
		parsePos++;
	} 
	if (!sectionStr || !entryStr) /*Valid Command*/
	{
		asDebugError("Command Invalid: %s", commandString);
		return AS_FAILURE_INVALID_PARAM;
	}

	/*Find Correct Preference Manager*/
	asPreferenceManager* pPrefMan = NULL;
	for (int i = 0; i < namespaceCount; i++)
	{
		if (asIsStringEqual(namespaceNames[i], namespaceStr))
		{
			pPrefMan = pNamespacePrefs[i];
			break;
		}
	}
	if (!pPrefMan)
	{
		asDebugError("Command Bad Namespace: %s", namespaceStr);
		return AS_FAILURE_DATA_DOES_NOT_EXIST;
	}

	/*Submit Pref*/
	asResults result;
	
	if (helpRequested) { result = asPreferencesPrintParamHelp(pPrefMan, sectionStr, entryStr); }
	else if (valueRequested) { result = asPreferencesPrintParamValue(pPrefMan, sectionStr, entryStr); }
	else { result = asPreferencesSetParam(pPrefMan, sectionStr, entryStr, arguementStr); asPreferencesPrintParamValue(pPrefMan, sectionStr, entryStr); }

	if (result == AS_FAILURE_DATA_DOES_NOT_EXIST)
	{
		asDebugError("Command Not Found: %s", commandString);
		return AS_FAILURE_DATA_DOES_NOT_EXIST;
	}
	else if (result == AS_FAILURE_INVALID_PARAM)
	{
		asDebugError("Arguement Required but Not Given: %s", commandString);
		return AS_FAILURE_INVALID_PARAM;
	}
	else if (result == AS_FAILURE_UNKNOWN_FORMAT)
	{
		asDebugError("No Retrievable Value: %s", commandString);
		return AS_FAILURE_INVALID_PARAM;
	}

	return AS_SUCCESS;
}

ASEXPORT void asGuiToolCommandConsoleUI()
{
	igSetNextWindowSizeConstraints((ImVec2) { 400, 600 }, (ImVec2){ FLT_MAX, FLT_MAX }, NULL, NULL);
	if (igBegin("astrengine Console", NULL, 0))
	{
		/*Log*/
		{
			const float footer_height_to_reserve = igGetStyle()->ItemSpacing.y + igGetFrameHeightWithSpacing();
			igBeginChild("ScrollingRegion", (ImVec2) { 0, -footer_height_to_reserve }, false, ImGuiWindowFlags_HorizontalScrollbar);

			_asDebugLoggerGetEntrySecureLogger();
			size_t entryCount = _asDebugLoggerGetEntryCount();

			/*Draw Log*/
			igPushStyleVarVec2(ImGuiStyleVar_ItemSpacing, (ImVec2) { 4, 1 });
			for (size_t i = 0; i < entryCount; i++)
			{
				asDebugLogSeverity severity;
				const char* msgContent;
				size_t msgLength;
				_asDebugLoggerGetEntryAtIdx(i, &severity, &msgContent, &msgLength);
				bool colorPopNecessary = false;
				switch (severity)
				{
				case AS_DEBUGLOG_WARNING: igPushStyleColor(ImGuiCol_Text, (ImVec4) { 1.0f, 0.7f, 0.0f, 1.0f }); colorPopNecessary = true; break;
				case AS_DEBUGLOG_ERROR: igPushStyleColor(ImGuiCol_Text, (ImVec4) { 1.0f, 0.2f, 0.0f, 1.0f }); colorPopNecessary = true; break;
				}

				igTextUnformatted(msgContent, msgContent + msgLength);
				if (colorPopNecessary)
					igPopStyleColor(1);
			}
			_asDebugLoggerGetEntryReleaseLogger();

			/*Scroll to Bottom*/
			if (igGetScrollY() >= igGetScrollMaxY())
				igSetScrollHereY(1.0f);

			igPopStyleVar(1);
			igEndChild();
		}

		/*Command Entry*/
		static char currentText[MAX_LINE_INPUT];
		bool reclaimCommandFocus = false;
		if (igInputText("Command", currentText, MAX_LINE_INPUT,
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_AlwaysInsertMode,
			_commandEntryCallback, currentText))
		{
			reclaimCommandFocus = true;
			enterCommand(currentText, false);
			memcpy(commandHistory[commandHistoryPoint % COMMAND_HISTORY], currentText, MAX_LINE_INPUT);
			commandHistoryPoint++;
			commandHistoryNewest = commandHistoryPoint;
			memset(currentText, 0, MAX_LINE_INPUT);
		}
		if (igIsItemHovered(0) && igIsMouseClicked(ImGuiMouseButton_Right, false))
		{
			igOpenPopup("CONSOLE_SUGGESTIONS");
		}

		/*Suggestions*/
		if (igBeginPopup("CONSOLE_SUGGESTIONS", 0))
		{
			for (size_t n = 0; n < namespaceCount; n++) /*Namespaces*/
			{
				if (igBeginMenu(namespaceNames[n], true))
				{
					size_t sectionCount = asPreferencesInspectGetSectionCount(pNamespacePrefs[n]);
					for (size_t s = 0; s < sectionCount; s++)
					{
						const char* sectionName = NULL;
						asPreferencesInspectGetSectionNameTmp(pNamespacePrefs[n], s, &sectionName);
						if (igBeginMenu(sectionName, true))
						{
							size_t entryCount = asPreferencesInspectGetEntryCount(pNamespacePrefs[n], s);
							for (size_t e = 0; e < entryCount; e++)
							{
								const char* entryName = NULL;
								asPreferencesInspectGetEntryNameTmp(pNamespacePrefs[n], s, e, &entryName);
								if (igMenuItemBool(entryName, NULL, false, true))
								{
									memset(currentText, 0, MAX_LINE_INPUT);
									snprintf(currentText, MAX_LINE_INPUT, "%s.%s.%s",
										namespaceNames[n],
										sectionName,
										entryName);
									reclaimCommandFocus = true;
								}
							}
							igEndMenu();
						}
					}
					igEndMenu();
				}
			}
			igEndPopup();
		}

		if (reclaimCommandFocus)
			igSetKeyboardFocusHere(-1);
	}
	igEnd();
}

ASEXPORT void asGuiToolCommandConsole_ExecuteCommand(const char* command)
{
	return enterCommand(command, true);
}
