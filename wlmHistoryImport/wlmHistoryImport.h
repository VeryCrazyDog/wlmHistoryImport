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
 * FileName: wlmHistoryImport.h
 * Description: This file contains defination of constants, macro and include header file.
 *
 */

#define MIRANDA_VER 0x0800

#if !defined( _UNICODE ) && defined( UNICODE )
	#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>

#include <newpluginapi.h>
#include <m_langpack.h>
#include <m_system.h>
#include <m_protocols.h>
#pragma warning(disable : 4819)
#include <m_clist.h>
#pragma warning(default : 4819)
#include <m_utils.h>
#include "XMLParser\wlm_xml.h"

// Service for menu item
#define WLM_IMPORT_SERVICE "WLMImport/Import"
// Maximum length of account module name
// Size of the account name is determine from file "protoopts.cpp" in Miranda IM
// main project
#define MAX_ACC_MOD_NAME_LEN 200

////////////////////////////////////////////////////////////////////////////////
//                              Short-cut macro                               //
////////////////////////////////////////////////////////////////////////////////
#define SIZEOF(x) (sizeof(x) / sizeof(x[0]))

////////////////////////////////////////////////////////////////////////////////
//                         Self-defined message type                          //
////////////////////////////////////////////////////////////////////////////////

// Message for enable a button
// wParam = Resource ID of the button
#define WIZM_ENABLEBUTTON     (WM_USER + 1)
// Message for disable a button
// wParam = Resource ID of the button
#define WIZM_DISABLEBUTTON    (WM_USER + 2)
// Message to go to specified page in the dialog
// wParam = Resource ID
// lParam = Dialog procedure function
#define WIZM_GOTOPAGE         (WM_USER + 3)	
// Message for starting the import process
#define IMPORT_START          (WM_USER + 4)
// Message to add a message in progress page of the wizard
// lParam = (char*)szText
#define PROGM_ADDMESSAGE      (WM_USER + 5)
// Message to set the progress bar in progress page of the wizard
// wParam = 0..100
#define PROGM_SETPROGRESS     (WM_USER + 6)
// Message to set the cancel button text
// lParam = (char*)newText
#define WIZM_SETCANCELTEXT    (WM_USER + 7)

////////////////////////////////////////////////////////////////////////////////
//                                Protocol Name                               //
////////////////////////////////////////////////////////////////////////////////

#define MSN_PROTO_NAME     "MSN"

//////////////////////////////////////////////////////////////////////////////////
////                                Import Options                              //
//////////////////////////////////////////////////////////////////////////////////

#define IOPT_IN_MSG      1
#define IOPT_OUT_MSG     2
//#define IOPT_IN_FILE     4
//#define IOPT_OUT_FILE    8
