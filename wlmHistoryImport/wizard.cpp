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
 * FileName: wizard.cpp
 * Description: This file contains functions for the GUI wizard. Functions here
 * are modified based on Import plugin for Miranda IM.
 *
 */

#include "wlmHistoryImport.h"
#include "resource.h"

#include <time.h>
// Undefine the "Translate" macro in "m_langpack.h"
// Workaround for the C4002 warning: too many actual parameters for macro 'Translate'
// when including "shlobj.h"
#undef Translate
#include <shlobj.h>
// Redefine "Translate" that was undefined before
#define Translate(s)   ((char*)CallService(MS_LANGPACK_TRANSLATESTRING,0,(LPARAM)(s)))

// Function declaration for function in "commonFunction.cpp"
TCHAR *replaceVariable(TCHAR *toReplace);
// Function declaration for function in "wlmImporter.cpp"
void wlmImport(HWND hdlgProgressWnd);
// Forward function declaration for functions in this file
BOOL CALLBACK wizardIntroPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK importAccountPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wlmDBProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wlmOptionsPageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);

// External variable in "wlmHistoryImport.cpp"
extern HINSTANCE hInst;
// External variable in "wlmImporter.cpp"
extern char importAccModuleName[];
extern TCHAR importFolderPath[];
extern int nImportOptions;
extern DWORD dwSinceDate;

// Variable used in the file
static HWND hdlgProgress;
// Pointer to the function for importing the message history
void (*importFunction)(HWND);

// This add a message to the list box in progress page of the wizard
void AddMessage(const char *fmt, ...) {
	va_list args;
	char msgBuf[4096];
	va_start(args, fmt);

	mir_vsnprintf(msgBuf, SIZEOF(msgBuf), Translate(fmt), args);
	#if defined ( _UNICODE )
	{
		TCHAR* str = mir_a2t(msgBuf);
		SendMessage(hdlgProgress, PROGM_ADDMESSAGE, 0, (LPARAM)str);
		mir_free(str);
	}
	#else
		SendMessage(hdlgProgress, PROGM_ADDMESSAGE, 0, (LPARAM)msgBuf);
	#endif
}

// This prompts the user for confirmation of the import
BOOL confirmImport(HWND hdlg) {
	int answer;

	answer = MessageBox(hdlg, TranslateT("Are you sure to start importing?"), 
		TranslateT("WLM History Importer"), MB_YESNOCANCEL | MB_ICONQUESTION);
	return (answer == IDYES);
}

// Procedure function for the finish page in the wizard
BOOL CALLBACK finishedPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) {
	BOOL result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, IDC_BACK, 0);
			SendMessage(GetParent(hdlg), WIZM_SETCANCELTEXT, NULL, (LPARAM)TranslateT("Finish"));
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_IMPORTACC, (LPARAM)importAccountPageProc);
					break;
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
BOOL CALLBACK progressPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) {
	BOOL result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, IDC_BACK, 0);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, IDOK, 0);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, IDCANCEL, 0);
			SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			PostMessage(hdlg, IMPORT_START, NULL, NULL);
			result = TRUE;
			break;
		// Message for starting the import proccess
		case IMPORT_START:
			hdlgProgress = hdlg;
			importFunction(hdlg);
			ShowWindow(GetDlgItem(hdlg, IDC_IMPORTING), SW_HIDE);
			SendMessage(GetParent(hdlg), WIZM_ENABLEBUTTON, IDOK, 0);
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
				int i = SendDlgItemMessage(hdlg, IDC_STATUS,LB_ADDSTRING, 0, lParam);
				SendDlgItemMessage(hdlg,IDC_STATUS, LB_SETTOPINDEX, i, 0);
			}
			break;
		// Message set the progress bar
		case PROGM_SETPROGRESS:
			SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETPOS, wParam, 0);
			break;
	}
	return result;
}

BOOL CALLBACK wlmAdvOptionPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) {
	BOOL result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_MSG:
					CheckDlgButton(hdlg, IDC_IN_MSG, !IsDlgButtonChecked(hdlg,IDC_IN_MSG));
					CheckDlgButton(hdlg, IDC_OUT_MSG, !IsDlgButtonChecked(hdlg,IDC_OUT_MSG));
					break;
				case IDC_SINCE:
					EnableWindow(GetDlgItem(hdlg, IDC_DATETIMEPICKER), IsDlgButtonChecked(hdlg, IDC_SINCE));
					break;
				case IDC_BACK:
					PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_OPTIONS, (LPARAM)wlmOptionsPageProc);
					break;
				case IDOK:
					nImportOptions = 0;
					if(IsDlgButtonChecked(hdlg, IDC_IN_MSG)) {
						nImportOptions = nImportOptions | IOPT_IN_MSG;
					}
					if(IsDlgButtonChecked(hdlg, IDC_OUT_MSG)) {
						nImportOptions = nImportOptions | IOPT_OUT_MSG;
					}
					if(IsDlgButtonChecked(hdlg, IDC_SINCE)) {
						struct _SYSTEMTIME stSelectedDate = {0};

						if(DateTime_GetSystemtime(GetDlgItem(hdlg,IDC_DATETIMEPICKER), &stSelectedDate) == GDT_VALID) {
							struct tm tmSelectedDate = {0};

							tmSelectedDate.tm_mday = stSelectedDate.wDay;
							tmSelectedDate.tm_mon = stSelectedDate.wMonth - 1;
							tmSelectedDate.tm_year = stSelectedDate.wYear - 1900;
							dwSinceDate = mktime(&tmSelectedDate);
						}
						else {
							break;
						}
					}
					else {
						dwSinceDate = NULL;
					}
					if(confirmImport(hdlg)) {
						importFunction = wlmImport;
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

// Procedure function for the option page in the wizard
BOOL CALLBACK wlmOptionsPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) {
	BOOL result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			CheckDlgButton(hdlg, IDC_RADIO_ALL, BST_CHECKED);
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BACK:
					PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_WLMDB, (LPARAM)wlmDBProc);
					break;
				case IDOK:
					if (IsDlgButtonChecked(hdlg, IDC_RADIO_ALL)) {
						if(confirmImport(hdlg)) {
							nImportOptions = IOPT_IN_MSG | IOPT_OUT_MSG;
							dwSinceDate = NULL;
							importFunction = wlmImport;
							PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_PROGRESS, (LPARAM)progressPageProc);
						}
					}
					else if(IsDlgButtonChecked(hdlg, IDC_RADIO_CUSTOM)) {
						PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_ADVOPTIONS, (LPARAM)wlmAdvOptionPageProc);
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

// Procedure function for the message history location page in the wizard
BOOL CALLBACK wlmDBProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) {
	BOOL result = FALSE;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			{
				TCHAR *defaultPath;

				#ifdef _DEBUG
					defaultPath = replaceVariable(_T("C:\\Documents and Settings\\Very Crazy Dog\\My Documents\\我已接收的檔案\\Very Crazy Dog's Message History"));
				#else
					defaultPath = replaceVariable(_T("%USERPROFILE%\\My Received Files"));
				#endif			
				SetDlgItemText(hdlg, IDC_FOLDERNAME, defaultPath);
				mir_free(defaultPath);
			}
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
						binfo.ulFlags = BIF_USENEWUI | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;
						binfo.lpszTitle = TranslateT("Please select the Windows Live Messenger message history directory:");
						pidl = SHBrowseForFolder(&binfo);
						if(pidl != NULL) {
							// Get the name of the folder
							TCHAR path[MAX_PATH];

							if (SHGetPathFromIDList(pidl, path)) {
								SetDlgItemText(hdlg, IDC_FOLDERNAME, path);
							}
							GlobalFree(pidl);
						}
					}
					break;
				case IDC_BACK:
					PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_IMPORTACC, (LPARAM)importAccountPageProc);
					break;
				case IDOK:
					{
						TCHAR path[MAX_PATH];
						UINT pathLen;
						TCHAR *realPath;

						pathLen = GetDlgItemText(hdlg, IDC_FOLDERNAME, path, SIZEOF(path));
						_tcscat_s(path, MAX_PATH, _T("\\MessageLog.xsl"));
						realPath = replaceVariable(path);
						if(_taccess_s(realPath, 4)) {
							MessageBox(hdlg, 
								TranslateT("File 'MessageLog.xsl' is missing.\r\nIs this a Windows Live Messenger message history folder?"), 
								TranslateT("WLM History Importer"), MB_OK | MB_ICONINFORMATION);
						}
						else {
							// Remove the appended file name
							if(pathLen <= MAX_PATH) {
								path[pathLen] = '\0';
							}
							mir_free(realPath);
							realPath = replaceVariable(path);
							_tcscpy_s(importFolderPath, MAX_PATH, realPath);
							_tcscat_s(importFolderPath, MAX_PATH, _T("\\"));
							PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_OPTIONS, (LPARAM)wlmOptionsPageProc);
						}
						mir_free(realPath);
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

// Procedure function for the import account page in the wizard
BOOL CALLBACK importAccountPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) {
	BOOL result = FALSE;
	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			{
				PROTOACCOUNT **accs;
				int count;
				int i;
				HWND hList = GetDlgItem(hdlg, IDC_ACCLIST);
				BOOL selectedFirst = FALSE;

				ProtoEnumAccounts(&count, &accs);
				for(i = 0; i < count; i++) {
					if(strcmp(accs[i]->szProtoName, MSN_PROTO_NAME) == 0) {
						if(accs[i]->bIsEnabled == TRUE && accs[i]->bIsVisible == TRUE) {
							int curIdx = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)accs[i]->tszAccountName);
							SendMessage(hList, LB_SETITEMDATA, curIdx, (LPARAM)accs[i]);
							if(selectedFirst == FALSE) {
								ListBox_SetCurSel(hList, curIdx);
								selectedFirst = TRUE;
							}
						}
					}
				}
				// Disable the button if no account is available
				if(selectedFirst == FALSE) {
					SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, IDOK, 0);
					ShowWindow(GetDlgItem(hdlg, IDC_NOMSNACCOUNT), SW_SHOW);
				}
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
					{
						// Get the selected account name
						HWND hList = GetDlgItem(hdlg, IDC_ACCLIST);
						int selectedIdx;
						
						selectedIdx = ListBox_GetCurSel(hList);
						if (selectedIdx != LB_ERR) {
							PROTOACCOUNT *pa = (PROTOACCOUNT *)ListBox_GetItemData(hList, selectedIdx);

							if(strlen(pa->szModuleName) + 1 <= MAX_ACC_MOD_NAME_LEN) {
								strcpy_s(importAccModuleName, MAX_ACC_MOD_NAME_LEN, pa->szModuleName);
								PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_WLMDB, (LPARAM)wlmDBProc);
							}
							else {
								// Tell the user the account name is too long
								MessageBox(hdlg, TranslateT("Sorry the account name that you chose is too long..."), 
									TranslateT("WLM History Importer"), MB_OK | MB_ICONEXCLAMATION);
							}
						}
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
BOOL CALLBACK wizardIntroPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) {
	BOOL result = FALSE;
	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, IDC_BACK, 0);
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_IMPORTACC, (LPARAM)importAccountPageProc);
					break;
				case IDCANCEL:
					PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
					break;
			}
			break;
	}
	return result;
}

// Main windows message handler function for the wizard dialog
BOOL CALLBACK wizardDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	BOOL result = FALSE;
	static HWND hwndPage = NULL;

	switch(message) {
		// Message to initialize dialog
		case WM_INITDIALOG:
			TranslateDialogDefault(hDlg);
			hwndPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_INTRO), hDlg, wizardIntroPageProc);
			SetWindowPos(hwndPage, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			ShowWindow(hwndPage, SW_SHOW);
			ShowWindow(hDlg, SW_SHOW);
			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_WLMIMPORT)));
			result = TRUE;
			break;
		// Message when a button is pressed
		case WM_COMMAND:
			// Pass the button pressed message to the associated page
			SendMessage(hwndPage, WM_COMMAND, wParam, lParam);
			break;
		// Message to Enable a button in the dialog
		case WIZM_ENABLEBUTTON:
			switch(wParam) {
				case IDC_BACK:
				case IDOK:
				case IDCANCEL:
					EnableWindow(GetDlgItem(hDlg, wParam), TRUE);
					break;
			}
			break;
		// Message to disable a button in the dialog
		case WIZM_DISABLEBUTTON:
			switch(wParam) {
				case IDC_BACK:
				case IDOK:
				case IDCANCEL:
					EnableWindow(GetDlgItem(hDlg, wParam), FALSE);
					break;
			}
			break;
		// Message to go to specified page
		case WIZM_GOTOPAGE:
			DestroyWindow(hwndPage);
			EnableWindow(GetDlgItem(hDlg, IDC_BACK), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDCANCEL), TRUE);
			SetDlgItemText(hDlg, IDCANCEL, TranslateT("Cancel"));
			hwndPage = CreateDialog(hInst, MAKEINTRESOURCE(wParam), hDlg, (DLGPROC)lParam);
			SetWindowPos(hwndPage, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			ShowWindow(hwndPage, SW_SHOW);
			break;
		// Message to set the cancel button text
		case WIZM_SETCANCELTEXT:
			SetDlgItemText(hDlg, IDCANCEL, (TCHAR*)lParam);
			break;
		// Message to close the window
		case WM_CLOSE:
			DestroyWindow(hwndPage);
			DestroyWindow(hDlg);
			break;
	}
	return result;
}
