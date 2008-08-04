#define MIRANDA_VER 0x0700

#if !defined( _UNICODE ) && defined( UNICODE )
	#define _UNICODE
#endif

//#include <tchar.h>

#include <windows.h>
//#include <commctrl.h> // datetimepicker

//#include <malloc.h>
//#include <stdio.h>
//#include <stddef.h>
//#include <time.h>
//#include <io.h>

#include <newpluginapi.h>
#include <m_langpack.h>
#include <m_system.h>

#include <m_protocols.h>
//#include <m_protosvc.h>
//#include <m_protomod.h>
//#include <m_findadd.h>
#pragma warning(disable : 4819)
#include <m_clist.h>
#pragma warning(default : 4819)
#include <m_utils.h>
#include <m_database.h>
//#include <win2k.h>

// Module name
#define WLM_IMPORT_MODULE  "WLMImport"
// Service for menu item
#define WLM_IMPORT_SERVICE "WLMImport/Import"

//// Self-defined message type
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

//// Keys
//#define IMP_KEY_FR      "FirstRun"   // First run


//// Protocol Name
//#define ICQOSCPROTONAME  "ICQ"
#define MSN_PROTO_NAME     "MSN"
//#define YAHOOPROTONAME   "YAHOO"
//#define NSPPROTONAME     "NET_SEND"
//#define ICQCORPPROTONAME "ICQ Corp"
//#define AIMPROTONAME     "AIM"

// Import type
#define IMPORT_ALL      0
#define IMPORT_CUSTOM   1

//// Custom import options
//#define IOPT_ADDUNKNOWN 1
//#define IOPT_MSGSENT    2
//#define IOPT_MSGRECV    4
//#define IOPT_URLSENT    8
//#define IOPT_URLRECV    16

// For progress page of the wizard
void AddMessage( const char* fmt, ... );
#define SET_PROGRESS(n)  SendMessage(hdlgProgress,PROGM_SETPROGRESS,n,0)

//extern HWND hdlgProgress;
//
//extern DWORD nDupes, nContactsCount, nMessagesCount, nGroupsCount, nSkippedEvents, nSkippedContacts;

// Path of the folder where the WLM database is located
TCHAR importFolderPath[MAX_PATH];
// Pointer to the function for importing the message history
void (*importFunction)(HWND);
// This indicate what to import from the database.
int nImportType;
// Handle to progress page of the wizard
HWND hdlgProgress;
