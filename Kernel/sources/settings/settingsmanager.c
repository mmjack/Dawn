#include <settings/settingsmanager.h>
#include <stdlib.h>
#include <debug/debug.h>
#include <types/size_t.h>
#include <fs/vfs.h>

/*
	GLOBAL VARIABLES
*/

settingsEntry* settingsListStart = 0;

/*
	FUNCTION DEFINITIONS
*/

settingsEntry* settingsGetEntry(const char* Name);
void settingsCreateEntry(const char* Name, const char* Data);

/*
	PUBLIC FUNCTIONS
 */

/**
 * Initializes the settings manager
 * Loads base settings from /system/root/kernel.config
 **/
void initializeSettingsManager()
{
	DEBUG_PRINT("Initialized settings manager\n");
	settingsListStart = 0;

	parseConfigFile("./system/kernel.config");
}

/**
 * Executes a line, like a query and returns any results there may be
 */

const char* settingsExecuteLine(const char* Line)
{
	DEBUG_PRINT("SETTINGS: EXECUTE %s\n", Line);


	size_t length = strlen(Line);

	const char* FirstSpace = strchr(Line, ' ');

	if (FirstSpace == 0)
	{
		DEBUG_PRINT("SETTINGS: ERROR INVALID COMMAND\n");
		return "FAIL";
	}

	const char* SecondSpace = strchr(FirstSpace + 1, ' ');

	if (SecondSpace == 0)
	{
		DEBUG_PRINT("SETTINGS: ERROR INVALID COMMAND\n");
		return "FAIL";
	}

	const char* End = Line + length;


	if (*(FirstSpace + 1) != '=')
	{
		DEBUG_PRINT("SETTINGS: ERROR INVALID COMMAND\n");
		return "FAIL";
	}

	if (FirstSpace - Line == 0)
	{
		DEBUG_PRINT("SETTINGS: ERROR INVALID COMMAND\n");
		return "FAIL";
	}

	if (End - SecondSpace == 0)
	{
		DEBUG_PRINT("SETTINGS: ERROR INVALID COMMAND\n");
		return "FAIL";
	}

	char* Name = malloc(length + 2);
	memcpy(Name, Line, FirstSpace - Line);
	Name[FirstSpace - Line] = '\0';

	DEBUG_PRINT("SETTINGS: *%s*\n", Name);

	
	char* Data = malloc(length + 2);
	memcpy(Data, SecondSpace + 1, End - SecondSpace - 1);
	Data[End - SecondSpace - 1] = '\0';

	DEBUG_PRINT("SETTINGS: *%s*\n", Data);

	settingsEntry* oldEntry = settingsGetEntry(Name);

	if (oldEntry == 0)
	{
		//Create a new entry
		settingsCreateEntry(Name, Data);
	}
	else
	{
		//Reuse a old entry
		settingsModifyEntry(oldEntry, Data);
	}

	free(Name);
	free(Data);

	return "FINE";
}

/**
 * Reads the value of the specified settings entry
 **/

const char* settingsReadValue(const char* Name)
{
	settingsEntry* entry = settingsGetEntry(Name);

	if (entry == 0)
	{
		return "";
	}
	else
	{
		return entry->Data;
	}
}

/*
	PRIVATE FUNCTIONS
 */

void parseConfigFile(const char* Filename)
{
	fs_node_t* cfgNode = evaluatePath(Filename, init_vfs());

	if (cfgNode == 0) return;

	char cfgBuffer[1024];
	memset(cfgBuffer, 0, 1024);

	size_t length = cfgNode->length;
	size_t current = 0;
	size_t iter = 0;


	open_fs(cfgNode);


	while (1)
	{
		if (current >= cfgNode->length)
			break;

		read_fs(cfgNode, current, 1, cfgBuffer + iter);

		if (*(cfgBuffer + iter) == '\n')
		{
			*(cfgBuffer + iter) = '\0';
			iter = 0;

			DEBUG_PRINT("SETTINGS: PARSE LINE %s\n", cfgBuffer);
			settingsExecuteLine(cfgBuffer);

			memset(cfgBuffer, 0, 1024);
		}
		else
		{
			iter++;
		}
	
		current++;
	}

	close_fs(cfgNode);
}

settingsEntry* settingsGetEntry(const char* Name)
{
	if (settingsListStart != 0)
	{
		settingsEntry* iter = settingsListStart;

		while (iter->next != 0)
		{

			if (strcmp(Name, iter->Name) == 0)
			{
				break;
			}

			iter = iter->next;
		}

		//Double check
		if (strcmp(Name, iter->Name) != 0)
		{
			return 0;
		}

		return iter;

	}

	return 0;
}

settingsEntry* settingsGetLastEntry()
{
	if (settingsListStart != 0)
	{
		settingsEntry* iter = settingsListStart;

		while (iter->next != 0)
		{
			iter = iter->next;
		}

		return iter;

	}

	return 0;
}

void settingsModifyEntry(settingsEntry* Entry, const char* newData)
{
	if (Entry->Data != 0)
	{
		free(Entry->Data);
	}

	Entry->Data = malloc(strlen(newData) + 1);
	strcpy(Entry->Data, newData);

	return;
}

void settingsCreateEntry(const char* Name, const char* Data)
{

	//Allocate the settings entry
	settingsEntry* newEntry = malloc(sizeof(settingsEntry));
	memset(newEntry, 0, sizeof(settingsEntry));
	
	//Allocate memory for the settings entry name and data
	newEntry->Name = malloc(strlen(Name) + 1);
	newEntry->Data = malloc(strlen(Data) + 1);

	//Copy the name and data
	strcpy(newEntry->Name, Name);
	strcpy(newEntry->Data, Data);

	//Double check the next entry is pointed to null
	newEntry->next = 0;

	//Find the last entry
	settingsEntry* last = settingsGetLastEntry();

	if (last != 0)
	{
		//Tack this entry on to the end
		last->next = newEntry;
	}
	else
	{
		//Set settingsListStart to this new entry
		settingsListStart = newEntry;
	}

	DEBUG_PRINT("SETTINGS: Added new settings entry %s initialized to %s\n", Name, Data);

	return;
}
