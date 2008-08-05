/*

    Windows Live Messenger History Import Plugin
    Imports messages from Windows Live Messenger
    Copyright (C) 2008  Very Crazy Dog (VCD)

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
 * FileName: wizard.c
 * Description: This file contains functions for the GUI wizard. Functions here
 * are modified based on Import plugin for Miranda IM.
 *
 */

#include "wlmHistoryImport.h"
#include "resource.h"

#include <shlobj.h>

extern HINSTANCE hInst;

// Function declaration for function not in this file
BOOL isProtocolLoaded(char* pszProtocolName);
void wlmImport(HWND hdlgProgressWnd);

// Forward function declaration for functions in this file
BOOL CALLBACK wizardIntroPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK importTypePageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wlmPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

// This add a message to the list box in progress page of the wizard
void AddMessage( const char* fmt, ... )
{
	va_list args;
	char msgBuf[ 4096 ];
	va_start( args, fmt );

	mir_vsnprintf( msgBuf, sizeof(msgBuf), Translate(fmt), args );
	#if defined( _UNICODE )
	{
		TCHAR* str = mir_a2t( msgBuf );
		SendMessage( hdlgProgress, PROGM_ADDMESSAGE, 0, ( LPARAM )str );
		mir_free( str );
	}
	#else
		SendMessage( hdlgProgress, PROGM_ADDMESSAGE, 0, ( LPARAM )msgBuf );
	#endif
}

// Procedure function for the finish page in the wizard
BOOL CALLBACK finishedPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned short result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 0, 0);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 1, 0);
			SendMessage(GetParent(hdlg), WIZM_SETCANCELTEXT, 0, (LPARAM)TranslateT("Finish"));
			CheckDlgButton(hdlg, IDC_DONTLOADPLUGIN, BST_UNCHECKED);
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				// When "Finish" button is pressed
				case IDCANCEL:
					PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
					break;
			}
			break;
		}
	return result;
}

// Procedure function for the progress page in the wizard
BOOL CALLBACK progressPageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	unsigned short result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 0, 0);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 1, 0);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 2, 0);
			SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			PostMessage(hdlg, IMPORT_START, 0, 0);
			result = TRUE;
			break;
		// Message for starting the import proccess
		case IMPORT_START:
			importFunction(hdlg);
			ShowWindow(GetDlgItem(hdlg, IDC_IMPORTING), SW_HIDE);
			SendMessage(GetParent(hdlg), WIZM_ENABLEBUTTON,1,0);
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_FINISHED, (LPARAM)finishedPageProc);
					break;
				case IDCANCEL:
					PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
					break;
			}
			break;
		// Message to add a message to the list box to display to the user
		case PROGM_ADDMESSAGE:
			{
				int i = SendDlgItemMessage(hdlg,IDC_STATUS,LB_ADDSTRING,0,lParam);
				SendDlgItemMessage(hdlg,IDC_STATUS,LB_SETTOPINDEX,i,0);
				#ifdef _DEBUG
				{
					FILE *stream;
					stream = fopen("Import Debug.log", "a");
					fprintf(stream, "%s\n", (char*)lParam);
					fclose(stream);
				}
				#endif
			}
			break;
		// Message set the progress bar
		case PROGM_SETPROGRESS:
			SendDlgItemMessage(hdlg,IDC_PROGRESS,PBM_SETPOS,wParam,0);
			break;
	}
	return result;
}

// Procedure function for the option page in the wizard
BOOL CALLBACK wlmOptionsPageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	unsigned short result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			EnableWindow(GetDlgItem(hdlg, IDC_RADIO_ALL), TRUE);
			EnableWindow(GetDlgItem(hdlg, IDC_STATIC_ALL), TRUE);
			CheckDlgButton(hdlg, IDC_RADIO_ALL, BST_CHECKED);
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BACK:
					PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_WLMDB, (LPARAM)wlmPageProc);
					break;
				case IDOK:
					if (IsDlgButtonChecked(hdlg, IDC_RADIO_ALL)) {
						importFunction = wlmImport;
						nImportType = IMPORT_ALL;
						PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_PROGRESS, (LPARAM)progressPageProc);
					}
					break;
				case IDCANCEL:
					PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
					break;
			}
			break;
	}
	return result;
}

// Procedure function for the finish page in the wizard
BOOL CALLBACK wlmPageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	unsigned short result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BROWSER:
					{
						BROWSEINFO binfo;
						LPITEMIDLIST pidl;

						ZeroMemory(&binfo, sizeof(binfo));
						binfo.hwndOwner = hdlg;
						binfo.lpszTitle = TranslateT("Please select the Windows Live Messenger database directory:");
						pidl = SHBrowseForFolder(&binfo);
						if(pidl != NULL) {
							// Get the name of the folder
							TCHAR path[MAX_PATH];
							if (SHGetPathFromIDList(pidl, path)) {
								SetDlgItemText(hdlg, IDC_FOLDERNAME, path);
							}
							// TODO: Free memory used
							//IMalloc * imalloc = 0;
							//if (SUCCEEDED(SHGetMalloc(&imalloc))) {
							//	imalloc->Free(pidl);
							//	imalloc->Release();
							//}
						}
					}
					break;
				case IDC_BACK:
					PostMessage(GetParent(hdlg),WIZM_GOTOPAGE,IDD_IMPORTTYPE,(LPARAM)importTypePageProc);
					break;
				case IDOK:
					{
						TCHAR folderName[MAX_PATH];
						TCHAR fileName[MAX_PATH];

						GetDlgItemText(hdlg, IDC_FOLDERNAME, folderName, sizeof(folderName));
						GetDlgItemText(hdlg, IDC_FOLDERNAME, fileName, sizeof(fileName));
						_tcscat_s(fileName, MAX_PATH, _T("\\MessageLog.xsl"));
						if(_taccess(fileName, 4)) {
							MessageBox(hdlg, TranslateT("MessageLog.xsl missing. Is this a Windows Live Messenger database folder?"), TranslateT("WLM History Import"), MB_OK);
						}
						else {
							_tcscpy_s(importFolderPath, MAX_PATH, folderName);
							_tcscat_s(importFolderPath, MAX_PATH, _T("\\"));
							PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_OPTIONS, (LPARAM)wlmOptionsPageProc);
						}
					}
					break;
				case IDCANCEL:
					PostMessage(GetParent(hdlg),WM_CLOSE,0,0);
					break;
			}
			break;
	}
	return result;
}

// Procedure function for the import type page in the wizard
BOOL CALLBACK importTypePageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned short result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			// Disable history import if WLM isn't loaded.
			if (!isProtocolLoaded(MSN_PROTO_NAME)) {
				EnableWindow(GetDlgItem(hdlg, IDC_WLM), FALSE);
				ShowWindow(GetDlgItem(hdlg,IDC_MSNNOTLOADED), SW_SHOW);
				SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 1, 0);
			}
			else {
				CheckDlgButton(hdlg, IDC_WLM, BST_CHECKED);
				ShowWindow(GetDlgItem(hdlg,IDC_MSNNOTLOADED), SW_HIDE);
			}
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BACK:
					PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_INTRO, (LPARAM)wizardIntroPageProc);
					break;
				case IDOK:
					if(IsDlgButtonChecked(hdlg, IDC_WLM)) {
						PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_WLMDB, (LPARAM)wlmPageProc);
					}
					break;
				case IDCANCEL:
					PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
					break;
			}
			break;
	}
	return result;
}

// Procedure function for the introduction page in the wizard
BOOL CALLBACK wizardIntroPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned short result = FALSE;
	switch( message ) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 0, 0);
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_IMPORTTYPE, (LPARAM)importTypePageProc);
					break;
				case IDCANCEL:
					PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
					break;
			}
			break;
	}
	return result;
}

// Main procedure function for the wizard dialog
BOOL CALLBACK wizardDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndPage;
	unsigned short result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			hwndPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_INTRO), hdlg, wizardIntroPageProc);
			SetWindowPos(hwndPage, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
			ShowWindow(hwndPage, SW_SHOW);
			ShowWindow(hdlg, SW_SHOW);
			SendMessage(hdlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst,MAKEINTRESOURCE(IDI_IMPORT)));
			result = TRUE;
			break;
		// Message to disable a button in the dialog
		case WIZM_DISABLEBUTTON:
			switch(wParam) {
				case 0:
					EnableWindow(GetDlgItem(hdlg, IDC_BACK), FALSE);
					break;
				case 1:
					EnableWindow(GetDlgItem(hdlg, IDOK), FALSE);
					break;
				case 2:
					EnableWindow(GetDlgItem(hdlg, IDCANCEL), FALSE);
					break;
			}
			break;
		// Message to Enable a button in the dialog
		case WIZM_ENABLEBUTTON:
			switch(wParam) {
				case 0:
					EnableWindow(GetDlgItem(hdlg, IDC_BACK), TRUE);
					break;
				case 1:
					EnableWindow(GetDlgItem(hdlg, IDOK), TRUE);
					break;
				case 2:
					EnableWindow(GetDlgItem(hdlg, IDCANCEL), TRUE);
					break;
			}
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			// Pass the button pressed message to the associated page
			SendMessage(hwndPage, WM_COMMAND, wParam, lParam);
			break;
		// Message to go to specified page
		case WIZM_GOTOPAGE:
			DestroyWindow(hwndPage);
			EnableWindow(GetDlgItem(hdlg, IDC_BACK), TRUE);
			EnableWindow(GetDlgItem(hdlg, IDOK), TRUE);
			EnableWindow(GetDlgItem(hdlg, IDCANCEL), TRUE);
			SetDlgItemText(hdlg, IDCANCEL, TranslateT("Cancel"));
			hwndPage = CreateDialog(hInst, MAKEINTRESOURCE(wParam), hdlg, (DLGPROC)lParam);
			SetWindowPos(hwndPage, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
			ShowWindow(hwndPage, SW_SHOW);
			break;
		// Message to set the cancel button text
		case WIZM_SETCANCELTEXT:
			SetDlgItemText(hdlg, IDCANCEL, (TCHAR*)lParam);
			break;
		// Message to close the window
		case WM_CLOSE:
			DestroyWindow(hwndPage);
			DestroyWindow(hdlg);
			break;
	}
	return result;
}
