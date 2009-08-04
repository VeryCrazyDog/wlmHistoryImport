/*
Miranda plugin template, originally by Richard Hughes
http://miranda-icq.sourceforge.net/

This file is placed in the public domain. Anybody is free to use or
modify it as they wish with no restriction.
There is no warranty.
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

// {8BBC5C4A-1131-4488-8120-4144422AEB7E}
#define MIID_LF2CRLF { 0x8bbc5c4a, 0x1131, 0x4488, { 0x81, 0x20, 0x41, 0x44, 0x42, 0x2a, 0xeb, 0x7e } }
#define LF2CRLF_SERVICE "LF2CRLF/Convert"

HINSTANCE hInst;
PLUGINLINK *pluginLink;
struct MM_INTERFACE mmi;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	"LF to CRLF",
	PLUGIN_MAKE_VERSION(0,8,0,0),
	"This plugin converts line ending in the content of message events from line feed to carage return and line feed.",
	"Very Crazy Dog (VCD)",
	"Very_Crazy_Dog@yahoo.com.hk",
	"Copyright (C) 2009  Very Crazy Dog",
	"http://forums.miranda-im.org/showthread.php?t=19337",
	UNICODE_AWARE,
	0,
	MIID_LF2CRLF
};

static unsigned int doConvert() {
	HANDLE hContact = NULL;
	unsigned int convertedEvent;

	convertedEvent = 0;
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		DBEVENTINFO dbei;
		HANDLE hDbEvent;
		int oldBlobSize;
		int newBlobSize;

		ZeroMemory(&dbei,sizeof(dbei));
		dbei.cbSize=sizeof(dbei);
		dbei.pBlob=NULL;
		hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDFIRST,(WPARAM)hContact,0);
		oldBlobSize=0;
		while ( hDbEvent != NULL ) {
			int msgLen;

			msgLen=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
			newBlobSize = msgLen;
			if(newBlobSize>oldBlobSize) {
				dbei.pBlob=(PBYTE)mir_realloc(dbei.pBlob,newBlobSize);
				oldBlobSize=newBlobSize;
			}
			dbei.cbBlob = oldBlobSize;
			CallService( MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei );
			//{
			//	int i;
			//	unsigned int crCount;
			//	unsigned int lfCount;
			//	
			//	crCount = 0;
			//	lfCount = 0;
			//	for(i = 0; i < dbei.cbBlob; i++) {
			//		if(dbei.pBlob[i] == '\r') {
			//			crCount++;
			//		}
			//		else if(dbei.pBlob[i] == '\n') {
			//			lfCount++;
			//		}
			//	}
			//	if(crCount < lfCount) {
			//		if(newBlobSize + lfCount > oldBlobSize) {
			//			dbei.pBlob=(PBYTE)mir_realloc(dbei.pBlob,newBlobSize + lfCount);
			//			oldBlobSize=newBlobSize + lfCount;
			//		}
			//		dbei.cbBlob = oldBlobSize;
			//		ZeroMemory(dbei.pBlob, dbei.cbBlob);
			//		CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei );
			//		
			//		for(i = oldBlobSize - 1; i > 0 && lfCount > 0; i--) {
			//			dbei.pBlob[i] = dbei.pBlob[i - lfCount];
			//			if(dbei.pBlob[i] == '\n') {
			//				dbei.pBlob[i - 1] = '\r';
			//				lfCount--;
			//				i--;
			//			}
			//		}
			//		CallService(MS_DB_EVENT_DELETE, (WPARAM)hContact, (LPARAM)hDbEvent);
			//		CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei);
			//		convertedEvent++;
			//		hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDFIRST,(WPARAM)hContact,0);
			//	}
			//}

			{
				int i;
				unsigned int lfCount;
				
				lfCount = 0;
				for(i = 0; i < dbei.cbBlob; i++) {
					if(dbei.pBlob[i] == '\n') {
						if(i - 1 >= 0) {
							if(dbei.pBlob[i - 1] != '\r') {
								lfCount++;
							}
						}
						else {
							lfCount++;
						}
					}
				}
				if(lfCount > 0) {
					if(newBlobSize + lfCount > oldBlobSize) {
						dbei.pBlob=(PBYTE)mir_realloc(dbei.pBlob,newBlobSize + lfCount);
						oldBlobSize=newBlobSize + lfCount;
					}
					dbei.cbBlob = oldBlobSize;
					ZeroMemory(dbei.pBlob, dbei.cbBlob);
					CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei );
					dbei.cbBlob = msgLen + lfCount;
					// Replace LF with CRLF
					for(i = oldBlobSize - 1; i > 0 && lfCount > 0; i--) {
						dbei.pBlob[i] = dbei.pBlob[i - lfCount];
						if(dbei.pBlob[i] == '\n') {
							if(i - lfCount - 1 < 0 || dbei.pBlob[i - lfCount - 1] != '\r') {
								dbei.pBlob[i - 1] = '\r';
								lfCount--;
								i--;
							}
						}
					}
					//// Trim CRLF at the end
					//i = dbei.cbBlob - 2;
					//while(i > 0) {
					//	if(dbei.pBlob[i] == '\n' && dbei.pBlob[i - 1] == '\r') {
					//		dbei.pBlob[i - 1] = '\0';
					//		dbei.cbBlob = dbei.cbBlob - 2;
					//		i = i - 2;
					//	}
					//	else if(dbei.pBlob[i] == ' ') {
					//		dbei.pBlob[i] = '\0';
					//		dbei.cbBlob = dbei.cbBlob - 1;
					//		i = i - 1;
					//	}
					//	else {
					//		i = 0;
					//	}
					//}
					//// Trim one CRLF if any
					//if(oldBlobSize > 3) {
					//	if(dbei.pBlob[oldBlobSize - 2] == '\n' && dbei.pBlob[oldBlobSize - 2 - 1] == '\r') {
					//		dbei.pBlob[oldBlobSize - 2 - 1] = '\0';
					//		dbei.cbBlob = dbei.cbBlob - 2;
					//	}
					//}
					CallService(MS_DB_EVENT_DELETE, (WPARAM)hContact, (LPARAM)hDbEvent);
					CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei);
					convertedEvent++;
					hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDFIRST,(WPARAM)hContact,0);
				}
			}
			hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDNEXT,(WPARAM)hDbEvent,0);
		}
		mir_free(dbei.pBlob);
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	return convertedEvent;
}

static int PluginMenuCommand(WPARAM wParam, LPARAM lParam)
{
	int answer;

	answer = MessageBox(NULL, "Are you sure you want to convert LF to CRLF in all MSN history event?", "LF to CRLF", MB_YESNOCANCEL | MB_ICONQUESTION);
	if(answer == IDYES) {
		unsigned int convertedEvent;
		char command[ 1024 ];

		convertedEvent = doConvert();
		mir_snprintf( command, sizeof( command ), "Converted %d message event.", convertedEvent );
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

	pluginLink=link;
	mir_getMMI(&mmi);
	CreateServiceFunction(LF2CRLF_SERVICE,PluginMenuCommand);
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=-0x7FFFFFFF;
	mi.flags=0;
	mi.hIcon=LoadSkinnedIcon(SKINICON_OTHER_HISTORY);
	mi.pszName=LPGEN("&LF to CRLF...");
	mi.pszService=LF2CRLF_SERVICE;
	CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
	return 0;
}

int __declspec(dllexport) Unload(void)
{
	return 0;
}