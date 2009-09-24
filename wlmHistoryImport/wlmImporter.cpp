/*

    Windows Live Messenger History Import Plugin
    Imports message history from Windows Live Messenger
    Copyright (C) 2008 - 2009  Very Crazy Dog (VCD)

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
 * FileName: wlmImporter.cpp
 * Description: This file contains functions for importing Windows Live 
 * Messenger history. This file used XML parser from Frank Vanden Berghen for parsing XML.
 *
 */

#include "wlmHistoryImport.h"

#include <time.h>
#include <assert.h>

#include <m_database.h>

#include "StringList.h"

// Name of the account module that the history is going to import to
char importAccModuleName[MAX_ACC_MOD_NAME_LEN];
// Path of the folder where the WLM database is located
TCHAR importFolderPath[MAX_PATH];
// This indicates the filter to apply
int nImportOptions;
// This indicates the first date in which the events should be imported
DWORD dwSinceDate;

// All XML tags appear in Windows Live Messenger history database
#define TAG_MESSAGE			_T("Message")
#define TAG_INV				_T("Invitation")
#define TAG_INVRESP			_T("InvitationResponse")
#define TAG_JOIN			_T("Join")
#define TAG_LEAVE			_T("Leave")
#define TAG_FROM			_T("From")
#define TAG_TO				_T("To")
#define TAG_USER			_T("User")
#define TAG_APP				_T("Application")
#define TAG_TEXT			_T("Text")
#define TAG_FILE			_T("File")

// Some attribute name used in Windows Live Messenger history database
#define ATTR_TIME			_T("DateTime")
#define ATTR_NAME			_T("FriendlyName")

// Function Marco
#define SET_PROGRESS(n)  SendMessage(hdlgProgressWnd, PROGM_SETPROGRESS, n, 0)

// Function declaration for function in "wizard.cpp"
void AddMessage(const char *fmt, ...);
// Function declaration for function in "commonFunction.cpp"
int getLocalUTCOffset();
unsigned int getNumOfContact(const char *accModuleName);
BOOL getUserID(char *sID, const size_t idLen, const HANDLE hContact);
BOOL isDuplicateEvent(HANDLE hContact, DBEVENTINFO dbei);

// Number of duplicated message during the import
static unsigned long nDupes;
// Number of skipped message during import
static unsigned long nSkippedChat;
// Number of failed import message during import
static unsigned long nFailed;
// Number of filtered message during import
static unsigned long nFiltered;
// Number of message imported
static unsigned long nImportedMessagesCount;
// A list of user name that is belong to the owner of the current profile
static StringList userNameList;
// The UTC time offset of the local time
static int nUTCOffset;

// Return:
// 0 if only one distinct user is found
// 1 if no user is found
// 2 if more than one user is found
unsigned int getUserName(const HXML hUserGroup, TCHAR *buf, int bufLen) {
	unsigned int result;
	TCHAR *name;
	int count;

	result = 1;
	name = new TCHAR[bufLen];
	ZeroMemory(name, bufLen * sizeof(TCHAR));
	count = xi.getChildCount(hUserGroup);
	for(int i = 0; i < count; i++) {
		HXML hContact;

		hContact = xi.getChild(hUserGroup, i);
		if(_tcscmp(xi.getName(hContact), TAG_USER) == 0) {
			LPCTSTR nameInXML;

			nameInXML = xi.getAttrValue(hContact, ATTR_NAME);
			if(name[0] == '\0') {
				result = 0;
				_tcsncpy_s(name, bufLen, nameInXML, _TRUNCATE);
			}
			else {
				if(_tcscmp(name, nameInXML) != 0) {
					// Invalid event from multiple users
					result = 2;
					// Exit the loop
					i = i + count;
				}
			}
		}
	}
	if(result == 0) {
		_tcsncpy_s(buf, bufLen, name, _TRUNCATE);
	}
	delete name;
	return result;
}

int promptNameSelection(const TCHAR *name1, const TCHAR *name2) {
	TCHAR translated[192];
	TCHAR question[512];

	ZeroMemory(translated, sizeof(translated));
	ZeroMemory(question, sizeof(question));
	_tcsncpy_s(translated, 192, TranslateT("Please indicate your name:\r\n"), _TRUNCATE);
	_tcscat_s(question, SIZEOF(question), translated);
	_tcsncpy_s(translated, 32, TranslateT("\r\nYES: "), _TRUNCATE);
	_tcscat_s(question, SIZEOF(question), translated);
	_tcscat_s(question, SIZEOF(question), name1);
	_tcsncpy_s(translated, 32, TranslateT("\r\nNO: "), _TRUNCATE);
	_tcscat_s(question, SIZEOF(question), translated);
	_tcscat_s(question, SIZEOF(question), name2);
	_tcsncpy_s(translated, 192, TranslateT("\r\n\r\nCancel: Abort the history import"), _TRUNCATE);
	_tcscat_s(question, SIZEOF(question), translated);
	return MessageBox(NULL, question, TranslateT("WLM History Importer"), MB_YESNOCANCEL | MB_ICONQUESTION);
}

// Return: TRUE if continue to import, FALSE to abort import
BOOL addMessageEvent(const HXML hEvent, const HANDLE hContact) {
	DBEVENTINFO eventInfo;
	TCHAR fromName[32];
	TCHAR toName[32];

	// Initialize the event info
	ZeroMemory(&eventInfo, sizeof(eventInfo));
	eventInfo.cbSize = sizeof(eventInfo);
	eventInfo.eventType = EVENTTYPE_MESSAGE;
	eventInfo.szModule = importAccModuleName;

	// Initalize other variables
	ZeroMemory(fromName, sizeof(fromName));
	ZeroMemory(fromName, sizeof(toName));

	// Get the time
	{
		LPCTSTR timeStr;
		struct tm time;
	
		timeStr = xi.getAttrValue(hEvent, ATTR_TIME);
		ZeroMemory(&time, sizeof(time));
		_stscanf_s(timeStr, _T("%d-%d-%dT%d:%d:%d"), &time.tm_year, &time.tm_mon, &time.tm_mday, &time.tm_hour, &time.tm_min, &time.tm_sec);
		time.tm_year = time.tm_year - 1900;
		time.tm_mon = time.tm_mon - 1;
		eventInfo.timestamp = mktime(&time);
		eventInfo.timestamp = eventInfo.timestamp + nUTCOffset;
	}
	if(dwSinceDate != 0 && (eventInfo.timestamp < dwSinceDate)) {
		// The event date is older than the specified date
		nFiltered = nFiltered + 1;
		return TRUE;
	}

	// Get the from contact
	{
		HXML hFrom;

		hFrom = xi.getNextChild(hEvent, TAG_FROM, NULL);
		if(getUserName(hFrom, fromName, SIZEOF(fromName)) != 0) {
			nFailed++;
			return TRUE;
		}
	}
	// Get the to contact
	{
		HXML hTo;
		unsigned int returnedValue;

		hTo = xi.getNextChild(hEvent, TAG_TO, NULL);
		returnedValue = getUserName(hTo, toName, SIZEOF(toName));
		switch(returnedValue) {
			case 0:
				break;
			case 1:
				nFailed++;
				return TRUE;
				break;
			case 2:
				nSkippedChat++;
				return TRUE;
				break;
			default:
				assert(TRUE);
				break;
		}
	}
	// Determine whether it is a send message or received message
	if(_tcscmp(fromName, toName) == 0) {
		nFailed++;
		return TRUE;
	}
	else if(userNameList.contain(fromName)) {
		eventInfo.flags = DBEF_SENT | DBEF_UTF;
	}
	else if(userNameList.contain(toName)) {
		eventInfo.flags = DBEF_READ | DBEF_UTF;
	}
	else {
		int answer;

		answer = promptNameSelection(fromName, toName);
		if(answer == IDCANCEL) {
			AddMessage(LPGEN("Aborting message import..."));
			return FALSE;
		}
		else {
			if(answer == IDYES) {
				eventInfo.flags = DBEF_SENT | DBEF_UTF;
				userNameList.addToFront(fromName);
			}
			else if(answer == IDNO) {
				eventInfo.flags = DBEF_READ | DBEF_UTF;
				userNameList.addToFront(toName);
			}
			else {
				assert(TRUE);
			}
		}
	}
	if(!(nImportOptions & IOPT_IN_MSG) && (eventInfo.flags & DBEF_READ) ) {
		nFiltered = nFiltered + 1;
		return TRUE;
	}
	if(!(nImportOptions & IOPT_OUT_MSG) && (eventInfo.flags & DBEF_SENT) ) {
		nFiltered = nFiltered + 1;
		return TRUE;
	}

	// Get the content of the message
	{
		HXML hText;
		LPCTSTR content;

		hText = xi.getNextChild(hEvent, TAG_TEXT, NULL);
		content = xi.getText(hText);
		if(content != NULL) {
			eventInfo.cbBlob = WideCharToMultiByte(CP_UTF8, NULL, content, -1, NULL, 0, NULL, NULL);
			eventInfo.pBlob = (PBYTE)malloc(eventInfo.cbBlob);
			ZeroMemory(eventInfo.pBlob, eventInfo.cbBlob);
			WideCharToMultiByte(CP_UTF8, NULL, content, -1, (LPSTR)eventInfo.pBlob, eventInfo.cbBlob, NULL, NULL);
			eventInfo.pBlob[eventInfo.cbBlob - 1] = 0;
		}
		else {
			// This message is an empty message
			eventInfo.cbBlob = 2;
			eventInfo.pBlob = (PBYTE)malloc(eventInfo.cbBlob);
			eventInfo.pBlob[0] = ' ';
			eventInfo.pBlob[1] = '\0';
		}
	}
	// Check for duplicate entries
	if(isDuplicateEvent(hContact, eventInfo)) {
		nDupes = nDupes + 1;
	}
	else {
		if(CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&(eventInfo))) {
			nImportedMessagesCount = nImportedMessagesCount + 1;
		}
		else {
			nFailed = nFailed + 1;
		}
	}
	free(eventInfo.pBlob);
	return TRUE;
}

// Return: 0 on success, 1 on abort, -1 on failure
int importParsedXML(const HXML hFile, const HANDLE hContact) {
	int result = 0;
	int eventCount;

	eventCount = xi.getChildCount(hFile);
	for(int i = 0; i < eventCount; i++) {
		HXML hEvent;

		hEvent = xi.getChild(hFile, i);
		if(_tcscmp(xi.getName(hEvent), TAG_MESSAGE) == 0) {
			if(addMessageEvent(hEvent, hContact) == FALSE) {
				// User choose to abort import
				return 1;
			}
		}
		else if(_tcscmp(xi.getName(hFile), TAG_INV) == 0) {
		}
		else if(_tcscmp(xi.getName(hFile), TAG_INVRESP) == 0) {
		}
		else {
			// Unknown tag, do nothing
		}
	}
	return result;
}

// This imports history from XML file in WLM format into Miranda IM
// Param: [in]sXMLPath - Path of the XML file to import
//        [in]hContact - Handle to the contact that associate with the XML file
// Return: 0 on success, 1 on abort, -1 on failure
int importXMLHistory(const TCHAR *sXMLPath, const HANDLE hContact) {
	int result = -1;
	FILE *fxmlFile;

	if(_tfopen_s(&fxmlFile, sXMLPath, _T("rb")) != 0) {
		AddMessage(LPGEN("Could not open file..."));
		result = -1;
	}
	else {
		long fileSize;

		fseek(fxmlFile, 0, SEEK_END);
		fileSize = ftell(fxmlFile);
		if(fileSize == -1) {
			AddMessage(LPGEN("Failed to find the size of file..."));
			result = -1;
		}
		else if(fileSize > 16 * 1024 * 1024) {
			AddMessage(LPGEN("The file size is larger than 16MB..."));
			result = -1;
		}
		else {
			char *buf;
			TCHAR *wBuf;
			int requiredSize;
			HXML hXML;

			// Load the file
			fseek(fxmlFile, 0, SEEK_SET);
			buf = new char[fileSize + 1];
			fread(buf, fileSize, 1, fxmlFile);
			requiredSize = MultiByteToWideChar(CP_UTF8, NULL, (char *)buf, -1, NULL, 0);
			wBuf = new TCHAR[requiredSize];
			MultiByteToWideChar(CP_UTF8, NULL, (char *)buf, -1, wBuf, requiredSize);
			// Unload the loaded file in UTF-8
			delete buf;
			// Parse the file
			hXML = xi.parseString(wBuf, NULL, _T("Log"));
			// Free the loaded file in TCHAR
			delete wBuf;
			// Import the parsed file
			result = importParsedXML(hXML, hContact);
			// Free the parsed file
			xi.destroyNode(hXML);
		}
		fclose(fxmlFile);
	}
	return result;
}

// This obtains the string before the '@' character in an email address
// Param: [out]sID - Buffer for putting a string before the '@' character of
//                   sEmail
//        [in]idLen - Length of the variable sID
//        [in]sEmail - Email address to proccess
//        [in]emailLen - Length of the variable sEmail
void getAccountNameFromEmail(TCHAR *sID, const size_t idLen, const char *sEmail, const size_t emailLen) {
	unsigned int count = 0;

	// Count number of character before '@'
	while(sEmail[count] != '\0' && sEmail[count] != '@' && count < emailLen) {
		count = count + 1;
	}
	// Copy the string before '@' to sID
	#if defined(_UNICODE)
		mbstowcs_s(NULL, sID, idLen, sEmail, count);
	#else
		sID[0] = '\0';
		strncat_s(sID, idLen, sEmail, count);
	#endif
}

// Main procedure for importing message history
void wlmImport(HWND hdlgProgressWnd) {
	PROTOACCOUNT *acc;

	HANDLE hContact = NULL;
	unsigned int nNumberOfContacts;
	unsigned int nImportedContactNum;

	nUTCOffset = getLocalUTCOffset();

	nNumberOfContacts = 0;
	nImportedContactNum = 0;
	nDupes = 0;
	nSkippedChat = 0;
	nFailed = 0;
	nFiltered = 0;
	nImportedMessagesCount = 0;
	userNameList.clear();

	// Checking before import history
	acc = ProtoGetAccount(importAccModuleName);
	if(acc == NULL) {
		AddMessage(LPGEN("The selected account does not exist."));
		AddMessage(LPGEN("Windows Live Messenger message history will not be imported."));
		SET_PROGRESS(100);
		return;
	}

	// Show number of contacts
	nNumberOfContacts = getNumOfContact(acc->szModuleName);
	AddMessage(LPGEN("Number of MSN contacts in current profile: %d"), nNumberOfContacts);
	AddMessage("");

	// Start import history for each MSN contacts in current profile
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		char *accBaseName = (char *)CallService(MS_PROTO_GETCONTACTBASEACCOUNT, (WPARAM)hContact, 0);
		if(accBaseName != NULL && strcmp(accBaseName, acc->szModuleName) == 0) {
			char sUserID[128];

			if(getUserID(sUserID, SIZEOF(sUserID), hContact) == TRUE) {
				// Email address of the MSN contact obtained
				TCHAR sFilePrefix[128];
				TCHAR sXMLPath[MAX_PATH];
				WIN32_FIND_DATA ffd;
				HANDLE hFind = INVALID_HANDLE_VALUE;
				unsigned int fileCount;

				getAccountNameFromEmail(sFilePrefix, SIZEOF(sFilePrefix), sUserID, SIZEOF(sUserID));
				// Prepare the XML path to look for
				mir_sntprintf(sXMLPath, SIZEOF(sXMLPath), _T("%s%s%s"), importFolderPath, sFilePrefix, _T("*.xml"));
				// Find the file count related to this contact
				{
					fileCount = 0;
					hFind = FindFirstFile(sXMLPath, &ffd);
					if(hFind != INVALID_HANDLE_VALUE) {
						do {
							fileCount++;
						} while(FindNextFile(hFind, &ffd) != 0);
						FindClose(hFind);
					}
				}
				if(fileCount > 0) {
					// Find the first XML file
					hFind = FindFirstFile(sXMLPath, &ffd);
					if(hFind != INVALID_HANDLE_VALUE) {
						AddMessage(LPGEN("Importing history for contact: %s"), sUserID);
						if(fileCount > 1) {
							AddMessage(LPGEN("%d files found for contact %s"), fileCount, sUserID);
						}
						do {
							// Prepare XML file path
							mir_sntprintf(sXMLPath, SIZEOF(sXMLPath), _T("%s%s"), importFolderPath, ffd.cFileName);
							// Import XML
							if(importXMLHistory(sXMLPath, hContact) == 1) {
								// Abort import
								goto END_IMPORT;
							}
						} while(FindNextFile(hFind, &ffd) != 0);
						FindClose(hFind);
					}
				}
			}
			// Update progress bar
			nImportedContactNum = nImportedContactNum + 1;
			SET_PROGRESS(100 * nImportedContactNum / nNumberOfContacts);
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
END_IMPORT:
	AddMessage("");
	AddMessage(LPGEN("%d events added."), nImportedMessagesCount);
	AddMessage(LPGEN("%d duplicated events skipped."), nDupes);
	AddMessage(LPGEN("%d events filtered."), nFiltered);
	AddMessage(LPGEN("%d chat log message skipped."), nSkippedChat);
	AddMessage(LPGEN("%d events failed to import."), nFailed);
	AddMessage("");
	AddMessage(LPGEN("Finished importing!"));
}
