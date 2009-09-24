/*

    LF to CRLF
    Converts line ending in all MSN history events from line feed to carriage 
    return and line feed.
    Copyright (C) 2009 - 2009  Very Crazy Dog (VCD)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#include <windows.h>
#include <newpluginapi.h>
#include <m_langpack.h>
#pragma warning(disable : 4819)
#include <m_clist.h>
#pragma warning(default : 4819)
#include <m_skin.h>
#include <m_database.h>
#include <m_system.h>
#include <m_protocols.h>

// GUID = {8BBC5C4A-1131-4488-8120-4144422AEB7E}
#define MIID_LF2CRLF { 0x8bbc5c4a, 0x1131, 0x4488, { 0x81, 0x20, 0x41, 0x44, 0x42, 0x2a, 0xeb, 0x7e } }
#define LF2CRLF_SERVICE "LF2CRLF/Convert"
#define MSN_PROTO_NAME "MSN"

HINSTANCE hInst;
PLUGINLINK *pluginLink;
struct MM_INTERFACE mmi;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	"LF to CRLF",
	PLUGIN_MAKE_VERSION(0,8,0,0),
	"This plugin converts line ending in all MSN history events from line feed to carage return and line feed.",
	"Very Crazy Dog (VCD)",
	"Very_Crazy_Dog@yahoo.com.hk",
	"Copyright (C) 2009 - 2009  Very Crazy Dog",
	"http://forums.miranda-im.org/showthread.php?t=19337",
	UNICODE_AWARE,
	0,
	MIID_LF2CRLF
};

static BOOL isBelongProtocol(HANDLE hContact, char *protocol) {	
	char *accBaseName;

	accBaseName = (char *)CallService(MS_PROTO_GETCONTACTBASEACCOUNT, (WPARAM)hContact, 0);
	return strcmp(accBaseName, MSN_PROTO_NAME) == 0;
}

static unsigned int countLF(PBYTE buf, DWORD bufSize) {
	unsigned int result = 0;
	DWORD i;

	for(i = 0; i < bufSize; i++) {
		if(buf[i] == '\n') {
			if(i - 1 >= 0) {
				if(buf[i - 1] != '\r') {
					result++;
				}
			}
			else {
				result++;
			}
		}
	}
	return result;
}

static void replaceLFbyCRLF(PBYTE buf, DWORD strLen, unsigned int lfCount) {
	int i;

	strLen = strLen + lfCount;
	for(i = strLen - 1; i > 0 && lfCount > 0; i--) {
		buf[i] = buf[i - lfCount];
		if(buf[i] == '\n') {
			if(i - lfCount - 1 < 0 || buf[i - lfCount - 1] != '\r') {
				buf[i - 1] = '\r';
				lfCount--;
				i--;
			}
		}
	}
}

static unsigned int doConvert() {
	HANDLE hContact = NULL;
	unsigned int convertedEvent;

	convertedEvent = 0;
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	// TODO: Check if the contact is MSN contact or not
	while (hContact != NULL) {
		if(isBelongProtocol(hContact, MSN_PROTO_NAME)) {
			DBEVENTINFO dbei;
			HANDLE hDbEvent;
			int currentBlobSize;

			ZeroMemory(&dbei, sizeof(dbei));
			dbei.cbSize = sizeof(dbei);
			dbei.pBlob = NULL;
			hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDFIRST, (WPARAM)hContact, 0);
			currentBlobSize = 0;
			while (hDbEvent != NULL) {
				int msgLen;
				int neededBlobSize;
				unsigned int lfCount;
				BOOL doneReplace = FALSE;

				// Check if the current size of the blob is large enough or not
				msgLen = CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
				neededBlobSize = msgLen;
				if(neededBlobSize > currentBlobSize) {
					dbei.pBlob = (PBYTE)mir_realloc(dbei.pBlob, neededBlobSize);
					currentBlobSize = neededBlobSize;
				}
				dbei.cbBlob = currentBlobSize;
				// Get the message event
				CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei);
				// Count the number of LF
				lfCount = countLF(dbei.pBlob, dbei.cbBlob);
				// Prepare to replace LF by CRLF if there is any LF in the event
				if(lfCount > 0) {
					// Calculate and obtain the new size needed
					neededBlobSize = msgLen + lfCount;
					if(neededBlobSize > currentBlobSize) {
						dbei.pBlob = (PBYTE)mir_realloc(dbei.pBlob, neededBlobSize);
						currentBlobSize = neededBlobSize;
						// Get the message again since the memory has been reallocated
						CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei);
					}
					// Prepare the size of the converted message
					dbei.cbBlob = neededBlobSize;
					// Replace LF with CRLF
					replaceLFbyCRLF(dbei.pBlob, msgLen, lfCount);
					{
						HANDLE hEventToDelete = hDbEvent;

						// Find the next message first
						hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDNEXT, (WPARAM)hDbEvent, 0);
						// Delete the old message
						CallService(MS_DB_EVENT_DELETE, (WPARAM)hContact, (LPARAM)hEventToDelete);
						// Add the converted one
						CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei);
						// Add the counter
						convertedEvent++;
						// Set the flag
						doneReplace = TRUE;
					}
				}
				if(doneReplace == FALSE) {
					hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDNEXT, (WPARAM)hDbEvent, 0);
				}
			}
			if(dbei.pBlob != NULL) {
				mir_free(dbei.pBlob);
			}
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	return convertedEvent;
}

static int PluginMenuCommand(WPARAM wParam, LPARAM lParam)
{
	int answer;

	answer = MessageBox(NULL, TranslateT("Are you sure you want to convert LF to CRLF in all MSN history event?\r\nThis will take a few minutes..."), "LF to CRLF", MB_YESNOCANCEL | MB_ICONQUESTION);
	if(answer == IDYES) {
		unsigned int convertedEvent;
		char command[ 1024 ];

		convertedEvent = doConvert();
		mir_snprintf( command, sizeof( command ), TranslateT("Converted %d message event."), convertedEvent );
		MessageBox(NULL, command, "LF to CRLF", MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 8, 0, 0))
		return NULL;

	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_TESTPLUGIN, MIID_LAST};
__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	CLISTMENUITEM mi;

	pluginLink = link;
	mir_getMMI(&mmi);
	CreateServiceFunction(LF2CRLF_SERVICE,PluginMenuCommand);
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.position = -0x7FFFFFFF;
	mi.flags = 0;
	mi.hIcon = LoadSkinnedIcon(SKINICON_OTHER_HISTORY);
	mi.pszName = LPGEN("&LF to CRLF...");
	mi.pszService = LF2CRLF_SERVICE;
	CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
	return 0;
}

int __declspec(dllexport) Unload(void)
{
	return 0;
}
