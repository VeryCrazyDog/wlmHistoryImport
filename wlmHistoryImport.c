/*
 * Problem
 * Done: Correctless of timestamp due to timezone and day light saving problem
 * Done: Correctless of duplicated message
 * Done: Empty string, string with space, line only
 * Done: Chinese character
 * Done: New line problem
 * Done: Sender, receiver
 * Done: Chat log
 * Incoming file
 * Incoming other invitation
 * Validate XML
 * Fix "Now Importing..." label missing in proccessing page
 *
 */


/*
 *
 * FileName: wlmHistoryImport.c
 * Description: This file contains functions for loading the plugin from DLL and also 
 * contains main functions for the operation of the plugin
 *
 */

#include "WLMHistoryImport.h"
#include "resource.h"

// Variables for DLL loading
HINSTANCE hInst;
PLUGINLINK *pluginLink;

// Variables for plugin
struct MM_INTERFACE mmi;
static HWND hwndWizard = NULL;

// Functions declaration for plugin
BOOL CALLBACK wizardDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	"Import WLM messages",
	PLUGIN_MAKE_VERSION(0,1,0,0),
	"Imports messages from Windows Live Messenger.",
	"Very Crazy Dog (VCD)",
	"Very_Crazy_Dog@yahoo.com.hk",
	"Copyleft 2008, Very Crazy Dog",
	"http://www.miranda-im.org/",
	UNICODE_AWARE,
	0,
	// UUID = {3bf4f550-1b3c-4739-a3db-c99eb912c422}
	{0x3bf4f550, 0x1b3c, 0x4739, { 0xa3, 0xdb, 0xc9, 0x9e, 0xb9, 0x12, 0xc4, 0x22 }}
};

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Functions for Plugin//////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

static int importCommand(WPARAM wParam,LPARAM lParam)
{
	if (IsWindow(hwndWizard)) {
		SetForegroundWindow(hwndWizard);
		SetFocus(hwndWizard);
	}
	else {
		hwndWizard = CreateDialog(hInst, MAKEINTRESOURCE(IDD_WIZARD), NULL, wizardDlgProc);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Functions for DLL Loading/////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

// MirandaPluginInfoEx - returns an information about a plugin
__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 7, 0, 0))
		return NULL;

	return &pluginInfo;
}

// MirandaPluginInterfaces - returns the protocol interface to the core
static const MUUID interfaces[] = {MIID_IMPORT, MIID_LAST};

__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

// Performs a primary set of actions upon plugin loading
int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;
	mir_getMMI( &mmi );

	CreateServiceFunction(WLM_IMPORT_SERVICE, importCommand);
	{
		CLISTMENUITEM mi;
		ZeroMemory(&mi, sizeof(mi));
		mi.cbSize = sizeof(mi);
		mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_IMPORT));
		mi.pszName = Translate("&WLM Import...");
		mi.position = 500050000;
		mi.pszService = WLM_IMPORT_SERVICE;
		CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi);
	}
	//HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	//{
	//	INITCOMMONCONTROLSEX icex;
	//	icex.dwSize = sizeof(icex);
	//	icex.dwICC = ICC_DATE_CLASSES;
	//	InitCommonControlsEx(&icex);
	//}

	return 0;
}

// Unload a plugin
int __declspec(dllexport) Unload(void)
{
	return 0;
}
