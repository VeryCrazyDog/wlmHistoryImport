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
 * FileName: wlmHistoryImport.h
 * Description: This file contains defination of constants and global variables.
 * Work modified based on Import plugin for Miranda IM.
 *
 */

#define MIRANDA_VER 0x0700

#if !defined( _UNICODE ) && defined( UNICODE )
	#define _UNICODE
#endif

#include <windows.h>

#include <newpluginapi.h>
#include <m_langpack.h>
#include <m_system.h>
#include <m_protocols.h>
#pragma warning(disable : 4819)
#include <m_clist.h>
#pragma warning(default : 4819)
#include <m_utils.h>
#include <m_database.h>

// Module name
#define WLM_IMPORT_MODULE  "WLMImport"
// Service for menu item
#define WLM_IMPORT_SERVICE "WLMImport/Import"


////////////////////////////////////////////////////////////////////////////////
//                         Self-defined message type                          //
////////////////////////////////////////////////////////////////////////////////

// Message for disable a button
// wParam = 0:Back, 1:Next, 2:Cancel
#define WIZM_DISABLEBUTTON  (WM_USER+11)
// Message for enable a button
// wParam = 0:Back, 1:Next, 2:Cancel
#define WIZM_ENABLEBUTTON   (WM_USER+13)
// Message to go to specified page in the dialog
// wParam = Resource ID
// lParam = Dialog procedure function
#define WIZM_GOTOPAGE    (WM_USER+10)	
// Message for starting the import process
#define IMPORT_START   (WM_USER+100)
// Message to set the cancel button text
// lParam = (char*)newText
#define WIZM_SETCANCELTEXT  (WM_USER+12)    
// Message to set the progress bar in progress page of the wizard
// wParam = 0..100
#define PROGM_SETPROGRESS  (WM_USER+10)
// Message to add a message in progress page of the wizard
// lParam = (char*)szText
#define PROGM_ADDMESSAGE   (WM_USER+11)


////////////////////////////////////////////////////////////////////////////////
//                                Protocol Name                               //
////////////////////////////////////////////////////////////////////////////////

#define MSN_PROTO_NAME     "MSN"

////////////////////////////////////////////////////////////////////////////////
//                                Import Options                              //
////////////////////////////////////////////////////////////////////////////////

#define IOPT_MSG    1
#define IOPT_FILE   2

////////////////////////////////////////////////////////////////////////////////
//                              Global Variables                              //
////////////////////////////////////////////////////////////////////////////////

// Path of the folder where the WLM database is located
TCHAR importFolderPath[MAX_PATH];
// Pointer to the function for importing the message history
void (*importFunction)(HWND);
// This indicates the filter to apply
int nImportOptions;
// This indicates the first date in which the events should be imported
DWORD dwSinceDate;
// Handle to progress page of the wizard
HWND hdlgProgress;
