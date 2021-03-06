/*

    Windows Live Messenger History Import Plugin
    Imports message history from Windows Live Messenger
    Copyright (C) 2008 - 2009  Very Crazy Dog (VCD)

    Based on
    Import plugin for Miranda IM
    Copyright (C) 2001,2002,2003,2004 Martin Öberg, Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

/*
 *
 * FileName: wlmHistoryImport.cpp
 * Description: This file contains functions for loading the plugin from DLL 
 * and unloading the plugin. This also contains entry functions for the 
 * operation of the plugin.
 *
 */

#include "WLMHistoryImport.h"
#include "resource.h"

// GUID = {3bf4f550-1b3c-4739-a3db-c99eb912c422}
#define MIID_WLMIMPORT { 0x3bf4f550, 0x1b3c, 0x4739, { 0xa3, 0xdb, 0xc9, 0x9e, 0xb9, 0x12, 0xc4, 0x22 } }

// Variables for DLL loading
HINSTANCE hInst;
PLUGINLINK *pluginLink;

// Variables for plugin
static HWND hwndWizard = NULL;
static HANDLE hImportService = NULL;
static HANDLE hHookOnExit = NULL;
MM_INTERFACE mmi;
XML_API xi;

// Functions declaration for plugin
BOOL CALLBACK wizardDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

// Plugin information
PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
	"WLM History Importer",
	PLUGIN_MAKE_VERSION(0,8,0,0),
	"Import history from Windows Live Messenger.",
	"Very Crazy Dog (VCD)",
	"Very_Crazy_Dog@yahoo.com.hk",
	"Copyright (C) 2008 - 2009  Very Crazy Dog",
	"http://forums.miranda-im.org/showthread.php?t=19337",
	UNICODE_AWARE,
	0,
	MIID_WLMIMPORT
};

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Functions for Plugin//////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

static int wlmImportCommand(WPARAM wParam,LPARAM lParam)
{
	if (IsWindow(hwndWizard)) {
		SetForegroundWindow(hwndWizard);
		SetFocus(hwndWizard);
	}
	else {
		// Create a dialog for user to use WLM import service
		hwndWizard = CreateDialog(hInst, MAKEINTRESOURCE(IDD_WIZARD), NULL, wizardDlgProc);
	}
	return 0;
}

static int OnExit(WPARAM wParam, LPARAM lParam)
{
	if (hwndWizard) {
		SendMessage(hwndWizard, WM_CLOSE, 0, 0);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Functions for DLL Loading/////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

// DLL main function
BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInfoEx - returns an information about a plugin
extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 8, 0, 0))
		return NULL;

	return &pluginInfoEx;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInterfaces - returns the protocol interface to the core
static const MUUID interfaces[] = {MIID_WLMIMPORT, MIID_LAST};

extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Performs a primary set of actions upon plugin loading
extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;
	mir_getMMI(&mmi);
	if(wlm_getXI(&xi) == FALSE) {
		MessageBox(NULL, TranslateT("Error on initializating XML parser.\r\nWLM History Importer will not be loaded."), 
			TranslateT("WLM History Importer"), MB_OK | MB_ICONERROR);
		return 1;
	}

	// Create a WLM import service
	hImportService = CreateServiceFunction(WLM_IMPORT_SERVICE, wlmImportCommand);
	// Add a item in the menu for user to call the WLM import service
	{
		CLISTMENUITEM mi;
		ZeroMemory(&mi, sizeof(mi));
		mi.cbSize = sizeof(mi);
		mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_WLMIMPORT));
		mi.pszName = Translate("&WLM Import...");
		mi.position = 500050000;
		mi.pszService = WLM_IMPORT_SERVICE;
		CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi);
	}
	// Hook the event when Miranda IM is ready to exit, for release resources
	hHookOnExit = HookEvent(ME_SYSTEM_OKTOEXIT, OnExit);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Unload the plugin
extern "C" int __declspec(dllexport) Unload(void)
{
	if (hHookOnExit) {
		UnhookEvent(hHookOnExit);
	}
	if (hImportService) {
		DestroyServiceFunction(hImportService);
	}
	return 0;
}
