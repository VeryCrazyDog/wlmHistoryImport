/*

    Windows Live Messenger History Import Plugin
    Imports messages from Windows Live Messenger
    Copyright (C) 2008  Very Crazy Dog (VCD)

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
 
/*===========================================================================
  Parsifal XML Parser
  Copyright (c) 2002-2005 Toni Uusitalo
  released to the public domain 2002-11-15
  http://www.saunalahti.fi/~samiuus/toni/xmlproc/

  Parsifal is free for both commercial and non-commercial use and
  redistribution, provided that author's copyright and disclaimer are
  retained intact.  You are free to modify Parsifal for your own use and
  to redistribute Parsifal with your modifications, provided that the
  modifications are clearly documented.

  DISCLAIMER
  ----------

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  Merchantability or fitness for a particular purpose. Please use it AT
  YOUR OWN RISK.
===========================================================================*/

/*
 *
 * FileName: wlmImporter.c
 * Description: This file contains functions for importing Windows Live 
 * Messenger history. This file used Parsifal XML Parser for parsing XML files.
 *
 */

#include "WLMHistoryImport.h"
#include "resource.h"

#include <time.h>
#include <stdio.h>
#include <assert.h>

#include "libparsifal/parsifal.h"
#pragma comment(lib, "libparsifal/parsifal.lib")

#include "tagStack.h"
#include "userNameList.h"

//// All XML tags appear in Windows Live Messenger history database
// Reserved value
#define TAG_OTHER		0
// Start of the log
#define TAG_LOG			1
// New message event
#define TAG_MESSAGE		2
// New invitation event / New incoming file event
#define TAG_INV			3
// New invitation response event / File received event / Send file fail event
#define TAG_INVRESP		4
// Chat room joined event
#define TAG_JOIN		5
// Chat room left event
#define TAG_LEAVE		6
// Start of sender details
#define TAG_FROM		7
// Start of receiver details
#define TAG_TO			8
// User name
#define TAG_USER		9
// Requested application name
#define TAG_APP			10
// Description of the event / Message of message event
#define TAG_TEXT		11
// Path of the file that is sent / received
#define TAG_FILE		12

// Information that is to be filled in in DBEVENTINFO
#define INFO_CBSIZE     1
#define INFO_MODULE     2
#define INFO_TIME       4
#define INFO_FLAGS      8
#define INFO_TYPE       16
#define INFO_CBBLOB     32
#define INFO_PBLOB      64

// Function Marco
#define SET_PROGRESS(n)  SendMessage(hdlgProgress,PROGM_SETPROGRESS,n,0)

// Function declaration for function not in this file
void AddMessage( const char* fmt, ... );
BOOL isProtocolLoaded(char* pszProtocolName);
BOOL isDuplicateEvent(HANDLE hContact, DBEVENTINFO dbei);
void utf8ToAnsi(const char *szIn, char *szOut, int cchOut);
void utf8ToWCHAR(const char *inString, WCHAR *outString, int outStringSize);

// Number of duplicated message during the import
DWORD nDupes;
// Number of skipped message during import
DWORD nSkippedChat;
// Number of failed import message during import
DWORD nFailed;
// Number of filtered message during import
DWORD nFiltered;
// Number of message imported
DWORD nImportedMessagesCount;
// A list of user name that is belong to the owner of the current profile
static list userNameList;

// This structure is used for variable passing when parsing XML files
typedef struct {
	// Tag stack used in XML parsing, only stack content will be changed during parsing.
	stack tagStack;
	// Handle to the contact, will not be changed during parsing
	HANDLE hContact;
	// This is the event type, possible values are:
	// TAG_MESSAGE, TAG_INV, TAG_INVRESP, TAG_JOIN, TAG_LEAVE
	// It will be changed during parsing
	int eventType;
	// Event info, will be changed during parsing
	DBEVENTINFO eventInfo;

	// The sender of the event, will be changed during parsing
	TCHAR fromContact[32];
	// The receiver of the event, will be changed during parsing
	TCHAR toContact[32];
	// Fields in eventInfo that is filled, will be changed during parsing
	int nFieldsFilled;
	// The UTC time offset of the local time, will not be changed during parsing
	int nUTCOffset;
	// For message event, it is possible that it is a chat log which involves more than two users.
	// This is used as a flag to indicate this event is chat log or not.
	// It will be changed during parsing
	int numOfContactsInvolved;
} xmlParseInfo;

// This returns the local UTC time offset in seconds
// Return: Offset between UTC+0 and local time in seconds
int getLocalUTCOffset() {
	TIME_ZONE_INFORMATION TimeZoneInformation;

	GetTimeZoneInformation(&TimeZoneInformation);
	return -TimeZoneInformation.Bias * 60;
}

// Callback function for XML parser to read data from file, see documentation
// from Parsifal XML Parser for more information
int xmlReadFileStream(BYTE *buf, int cBytes, int *cBytesActual, void *inputData)
{
	HANDLE hFile = (HANDLE)inputData;
	ReadFile(hFile, buf, cBytes, cBytesActual, NULL);
	return (*cBytesActual < cBytes);
}

// Callback function for XML parser when start tag is encountered, see 
// documentation from Parsifal XML Parser for more information
int xmlStartTagCallback(void *UserData, const XMLCH *uri, const XMLCH *localName, const XMLCH *qName, LPXMLVECTOR atts)
{
	xmlParseInfo *info = (xmlParseInfo *)UserData;

	if(strcmp(qName, "User") == 0) {
		switch(info->eventType) {
			case TAG_MESSAGE:
				{
					int i;
					TCHAR xmlContact[32];

					xmlContact[0] = '\0';
					// Get the new contact name
					for(i = 0; i < atts->length; i = i + 1) {
						LPXMLRUNTIMEATT att;

						att = (LPXMLRUNTIMEATT)XMLVector_Get(atts, i);
						if(strcmp(att->qname, "FriendlyName") == 0) {
							#if defined(_UNICODE)
								utf8ToWCHAR(att->value, xmlContact, sizeof(xmlContact) / sizeof(TCHAR));
							#else
								utf8ToAnsi(att->value, xmlContact, sizeof(xmlContact) / sizeof(TCHAR));
							#endif
							// Break the for loop
							break;
						}
					}
					if(info->numOfContactsInvolved == 0) {
						// Add the new contact name
						if(stack_Top(&info->tagStack) == TAG_FROM) {
							_tcsncpy_s(info->fromContact, sizeof(info->fromContact) / sizeof(TCHAR), xmlContact, _TRUNCATE);
							info->numOfContactsInvolved = info->numOfContactsInvolved + 1;
						}
						else if(stack_Top(&info->tagStack) == TAG_TO) {
							_tcsncpy_s(info->toContact, sizeof(info->toContact) / sizeof(TCHAR), xmlContact, _TRUNCATE);
							info->numOfContactsInvolved = info->numOfContactsInvolved + 1;
						}
					}
					else {
						// If the new contact name is not duplicate of the existing "From Contact" and "To Contact", and the field is not filled, add it
						// Number of contacts involved is increased by 1 even the fill is filled, because the message maybe a chat log
						if(_tcscmp(xmlContact, info->fromContact) != 0 && _tcscmp(xmlContact, info->toContact) != 0) {
							if(stack_Top(&info->tagStack) == TAG_FROM) {
								if((info->fromContact)[0] == '\0') {
									_tcsncpy_s(info->fromContact, sizeof(info->fromContact) / sizeof(TCHAR), xmlContact, _TRUNCATE);
								}
								info->numOfContactsInvolved = info->numOfContactsInvolved + 1;
							}
							else if(stack_Top(&info->tagStack) == TAG_TO) {
								if((info->toContact)[0] == '\0') {
									_tcsncpy_s(info->toContact, sizeof(info->toContact) / sizeof(TCHAR), xmlContact, _TRUNCATE);
								}
								info->numOfContactsInvolved = info->numOfContactsInvolved + 1;
							}
						}
					}
				}
				break;
		}
		stack_Push(&info->tagStack, TAG_USER);
	}
	else if(strcmp(qName, "Text") == 0) {
		stack_Push(&info->tagStack, TAG_TEXT);
	}
	else if(strcmp(qName, "From") == 0) {
		stack_Push(&info->tagStack, TAG_FROM);
	}
	else if(strcmp(qName, "To") == 0) {
		stack_Push(&info->tagStack, TAG_TO);
	}
	else if(strcmp(qName, "Message") == 0) {
		stack_Push(&info->tagStack, TAG_MESSAGE);
		// A new message event encountered
		if(atts->length > 0) {
			int i;

			assert(info->eventType == 0 && info->nFieldsFilled == 0 && info->numOfContactsInvolved == 0);
			// Initialize
			ZeroMemory(&info->eventInfo, sizeof(DBEVENTINFO));
			ZeroMemory(info->fromContact, sizeof(info->fromContact));
			ZeroMemory(info->toContact, sizeof(info->toContact));
			// Start fill in the info
			info->eventType = TAG_MESSAGE;
			// Start fill in the eventInfo
			info->eventInfo.cbSize = sizeof(DBEVENTINFO);
			info->nFieldsFilled = info->nFieldsFilled | INFO_CBSIZE;
			info->eventInfo.eventType = EVENTTYPE_MESSAGE;
			info->nFieldsFilled = info->nFieldsFilled | INFO_TYPE;
			info->eventInfo.szModule = MSN_PROTO_NAME;
			info->nFieldsFilled = info->nFieldsFilled | INFO_MODULE;
			// Find and parse the message time, and convert to timestamp
			for(i = 0; i < atts->length; i = i + 1) {
				LPXMLRUNTIMEATT att;

				att = (LPXMLRUNTIMEATT)XMLVector_Get(atts, i);
				if(strcmp(att->qname, "DateTime") == 0) {
					struct tm time;

					ZeroMemory(&time, sizeof(time));
					sscanf_s(att->value, "%d-%d-%dT%d:%d:%d", &time.tm_year, &time.tm_mon, &time.tm_mday, &time.tm_hour, &time.tm_min, &time.tm_sec);
					time.tm_year = time.tm_year - 1900;
					time.tm_mon = time.tm_mon - 1;
					info->eventInfo.timestamp = mktime(&time);
					info->eventInfo.timestamp = info->eventInfo.timestamp + info->nUTCOffset;
					info->nFieldsFilled = info->nFieldsFilled | INFO_TIME;
					// Break the for loop
					break;
				}
			}
		}
	}
	else if(strcmp(qName, "Invitation") == 0) {
		stack_Push(&info->tagStack, TAG_INV);
	}
	else if(strcmp(qName, "InvitationResponse") == 0) {
		stack_Push(&info->tagStack, TAG_INVRESP);
	}
	else if(strcmp(qName, "File") == 0) {
		stack_Push(&info->tagStack, TAG_FILE);
	}
	else if(strcmp(qName, "Application") == 0) {
		stack_Push(&info->tagStack, TAG_APP);
	}
	else if(strcmp(qName, "Join") == 0) {
		stack_Push(&info->tagStack, TAG_JOIN);
	}
	else if(strcmp(qName, "Leave") == 0) {
		stack_Push(&info->tagStack, TAG_LEAVE);
	}
	else if(strcmp(qName, "Log") == 0) {
		stack_Push(&info->tagStack, TAG_LOG);
	}
	return XML_OK;
}

// Callback function for XML parser when end tag is encountered, see
// documentation from Parsifal XML Parser for more information
int xmlEndTagCallback(void *UserData, const XMLCH *uri, const XMLCH *localName, const XMLCH *qName)
{
	xmlParseInfo *info = (xmlParseInfo *)UserData;
	int popedTag = stack_Pop(&info->tagStack);

	if(strcmp(qName, "User") == 0) {
		assert(popedTag == TAG_USER);
	}
	else if(strcmp(qName, "Text") == 0) {
		assert(popedTag == TAG_TEXT);
		if(stack_Top(&info->tagStack) == TAG_MESSAGE) {
			if(!(info->nFieldsFilled & INFO_PBLOB) && !(info->nFieldsFilled & INFO_CBBLOB)) {
				// This message is an empty message
				info->eventInfo.cbBlob = 2;
				info->nFieldsFilled = info->nFieldsFilled | INFO_CBBLOB;
				info->eventInfo.pBlob = (PBYTE)malloc(info->eventInfo.cbBlob);
				info->eventInfo.pBlob[0] = ' ';
				info->eventInfo.pBlob[1] = '\0';
				info->nFieldsFilled = info->nFieldsFilled | INFO_PBLOB;
			}
		}
	}
	else if(strcmp(qName, "From") == 0) {
		assert(popedTag == TAG_FROM);
	}
	else if(strcmp(qName, "To") == 0) {
		assert(popedTag == TAG_TO);
	}
	else if(strcmp(qName, "Message") == 0) {
		assert(popedTag == TAG_MESSAGE);
		// End of the message data
		if(!(nImportOptions & IOPT_MSG)) {
			// User do not want to import message event
			nFiltered = nFiltered + 1;
		}
		else if(dwSinceDate != 0 && (info->eventInfo.timestamp < dwSinceDate)) {
			// The event date is older than the specified date
			nFiltered = nFiltered + 1;
		}
		else if(info->numOfContactsInvolved == 2) {
			// Determine if the message is a sent message or received message
			if(list_Contains(&userNameList, info->fromContact)) {
				info->eventInfo.flags = DBEF_SENT | DBEF_UTF;
				info->nFieldsFilled = info->nFieldsFilled | INFO_FLAGS;
			}
			else if(list_Contains(&userNameList, info->toContact)) {
				info->eventInfo.flags = DBEF_READ | DBEF_UTF;
				info->nFieldsFilled = info->nFieldsFilled | INFO_FLAGS;
			}
			else {
				TCHAR translated[192];
				TCHAR question[512];
				int answer;

				ZeroMemory(translated, sizeof(translated));
				ZeroMemory(question, sizeof(question));
				_tcsncpy_s(translated, 192, TranslateT("Please indicate your name:\r\n"), _TRUNCATE);
				_tcscat_s(question, sizeof(question) / sizeof(TCHAR), translated);
				_tcsncpy_s(translated, 32, TranslateT("\r\nYES: "), _TRUNCATE);
				_tcscat_s(question, sizeof(question) / sizeof(TCHAR), translated);
				_tcscat_s(question, sizeof(question) / sizeof(TCHAR), info->fromContact);
				_tcsncpy_s(translated, 32, TranslateT("\r\nNO: "), _TRUNCATE);
				_tcscat_s(question, sizeof(question) / sizeof(TCHAR), translated);
				_tcscat_s(question, sizeof(question) / sizeof(TCHAR), info->toContact);
				_tcsncpy_s(translated, 192, TranslateT("\r\n\r\nCancel: Abort the history import"), _TRUNCATE);
				_tcscat_s(question, sizeof(question) / sizeof(TCHAR), translated);
				answer = MessageBox(NULL, question, TranslateT("WLM History Import"), MB_YESNOCANCEL);
				if(answer == IDCANCEL) {
					AddMessage(LPGEN("Aborting message import..."));
					free(info->eventInfo.pBlob);
					return XML_ABORT;
				}
				else {
					if(answer == IDYES) {
						info->eventInfo.flags = DBEF_SENT | DBEF_UTF;
						list_Add(&userNameList, info->fromContact);
						info->nFieldsFilled = info->nFieldsFilled | INFO_FLAGS;
					}
					else if(answer == IDNO) {
						info->eventInfo.flags = DBEF_READ | DBEF_UTF;
						list_Add(&userNameList, info->toContact);
						info->nFieldsFilled = info->nFieldsFilled | INFO_FLAGS;
					}
				}
			}
			
			if(info->nFieldsFilled == (INFO_CBSIZE | INFO_MODULE | INFO_TIME | INFO_FLAGS | INFO_TYPE | INFO_CBBLOB | INFO_PBLOB)) {
				// Check for duplicate entries
				if(isDuplicateEvent(info->hContact, info->eventInfo)) {
					nDupes = nDupes + 1;
				}
				else {
					if(CallService(MS_DB_EVENT_ADD, (WPARAM)info->hContact, (LPARAM)&(info->eventInfo))) {
						nImportedMessagesCount = nImportedMessagesCount + 1;
					}
					else {
						nFailed = nFailed + 1;
					}
				}
			}
			else {
				// Some fields missing
				nFailed = nFailed + 1;
			}
		}
		else {
			// This is a chat log
			nSkippedChat = nSkippedChat + 1;
		}
		if(info->nFieldsFilled & INFO_PBLOB) {
			free(info->eventInfo.pBlob);
		}
		info->eventType = 0;
		info->nFieldsFilled = 0;
		info->numOfContactsInvolved = 0;
	}
	else if(strcmp(qName, "Invitation") == 0) {
		assert(popedTag == TAG_INV);
	}
	else if(strcmp(qName, "InvitationResponse") == 0) {
		assert(popedTag == TAG_INVRESP);
	}
	else if(strcmp(qName, "File") == 0) {
		assert(popedTag == TAG_FILE);
	}
	else if(strcmp(qName, "Application") == 0) {
		assert(popedTag == TAG_APP);
	}
	else if(strcmp(qName, "Join") == 0) {
		assert(popedTag == TAG_JOIN);
	}
	else if(strcmp(qName, "Leave") == 0) {
		assert(popedTag == TAG_LEAVE);
	}
	else if(strcmp(qName, "Log") == 0) {
		assert(popedTag == TAG_LOG);
	}
	return XML_OK;
}

// Callback function for XML parser when data between tag is encountered, see
// documentation from Parsifal XML Parser for more information
int xmlDataCallback(void *UserData, const XMLCH *chars, int cbChars)
{
	xmlParseInfo *info = (xmlParseInfo *)UserData;

	if(info->eventType == TAG_MESSAGE) {
		if(stack_Top(&info->tagStack) == TAG_TEXT) {
			assert(info->eventInfo.pBlob == NULL);
			info->eventInfo.cbBlob = cbChars + 1;
			info->nFieldsFilled = info->nFieldsFilled | INFO_CBBLOB;
			info->eventInfo.pBlob = (PBYTE)malloc(info->eventInfo.cbBlob);
			CopyMemory(info->eventInfo.pBlob, chars, cbChars);
			info->eventInfo.pBlob[cbChars] = '\0';
			info->nFieldsFilled = info->nFieldsFilled | INFO_PBLOB;
		}
	}
	return XML_OK;
}

// This imports history from XML file in WLM format into Miranda IM
// Param: [in]sXMLPath - Path of the XML file to import
//        [in]hContact - Handle to the contact that associate with the XML file
// Return: 1 on success, 0 on abort, -1 on failure
int importXMLHistory(const TCHAR *sXMLPath, const HANDLE hContact) {
	HANDLE hFile;
	int result = -1;

	hFile = CreateFile(sXMLPath,
		GENERIC_READ,                 // open for reading
		0,                            // do not share
		NULL,                         // no security
		OPEN_EXISTING,                // existing file only
		FILE_ATTRIBUTE_NORMAL,        // normal file
		NULL);                        // no attr. template

	if (hFile == INVALID_HANDLE_VALUE) {
		AddMessage(LPGEN("Could not open file..."));
		result = -1;
	}
	else {
		LPXMLPARSER parser;
		xmlParseInfo info;

		XMLParser_Create(&parser);
		parser->startElementHandler = xmlStartTagCallback;
		parser->endElementHandler = xmlEndTagCallback;
		parser->charactersHandler = xmlDataCallback;
		ZeroMemory(&info, sizeof(info));
		stack_Init(&info.tagStack);
		info.hContact = hContact;
		info.nFieldsFilled = 0;
		info.nUTCOffset = getLocalUTCOffset();
		parser->UserData = (void *)&info;
		result = XMLParser_Parse(parser, xmlReadFileStream, (void *)hFile, NULL);
		XMLParser_Free(parser);
		CloseHandle(hFile);
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

// This returns the email address of a contact
// Param: [out]sEmail - Email address of the queried contact
//        [in]emailLen - Length of the variable sEmail
//        [in]hContact - Handle to the queried contact
// Return: TRUE when the email is obtained successfully, otherwise FALSE
int getEmail(char *sEmail, const size_t emailLen, const HANDLE hContact) {
	DBVARIANT dbv;
	DBCONTACTGETSETTING sVal;

	dbv.pszVal = sEmail;
	dbv.cchVal = (WORD)emailLen;
	dbv.type = DBVT_ASCIIZ;
	sVal.pValue = &dbv;
	sVal.szModule = MSN_PROTO_NAME;
	sVal.szSetting = "e-mail";
	return (CallService(MS_DB_CONTACT_GETSETTINGSTATIC, (WPARAM)hContact, (LPARAM)&sVal) == 0);
}

// This returns the number of MSN contact in current profile
// Return: Number of MSN contact in current profile
int getNumOfMSNContact() {
	HANDLE hContact = NULL;
	int nContactCount = 0;

	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if(szProto != NULL && strcmp(szProto, MSN_PROTO_NAME) == 0) {
			nContactCount = nContactCount + 1;
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	return nContactCount;
}

// Main procedure for importing message history
void wlmImport(HWND hdlgProgressWnd)
{
	HANDLE hContact = NULL;
	unsigned int nNumberOfContacts = 0;
	unsigned int nImportedContactNum = 0;

	nDupes = 0;
	nSkippedChat = 0;
	nFailed = 0;
	nFiltered = 0;
	nImportedMessagesCount = 0;
	list_Reset(&userNameList);

	// Just to keep the macros "SetProgress" happy
	hdlgProgress = hdlgProgressWnd;

	// Checking before import history
	if(!isProtocolLoaded(MSN_PROTO_NAME)) {
		AddMessage(LPGEN("MSN plugin is not installed."));
		AddMessage(LPGEN("Windows Live Messenger history will be imported."));
		SET_PROGRESS(100);
		return;
	}

	nNumberOfContacts = getNumOfMSNContact();
	AddMessage(LPGEN("Number of MSN contacts in current profile: %d"), nNumberOfContacts);

	// Start import history for each MSN contacts in current profile
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if(szProto != NULL && strcmp(szProto, MSN_PROTO_NAME) == 0) {
			char sEmail[128];

			if(getEmail(sEmail, sizeof(sEmail), hContact) == TRUE) {
				// Email address of the MSN contact obtained
				unsigned int count = 0;
				TCHAR sID[128];
				TCHAR sImportXML[MAX_PATH];
				WIN32_FIND_DATA ffd;
				HANDLE hFind = INVALID_HANDLE_VALUE;

				getAccountNameFromEmail(sID, sizeof(sID) / sizeof(TCHAR), sEmail, sizeof(sEmail));
				// Prepare the XML path to look for
				_tcscpy_s(sImportXML, MAX_PATH, importFolderPath);
				_tcscat_s(sImportXML, MAX_PATH, sID);
				_tcscat_s(sImportXML, MAX_PATH, _T("*.xml"));
				// Find the first XML file
				hFind = FindFirstFile(sImportXML, &ffd);
				if(hFind != INVALID_HANDLE_VALUE) {
					AddMessage(LPGEN("Importing history for contact: %s"), sEmail);
					do {
						// Prepare XML file path
						_tcscpy(sImportXML, importFolderPath);
						_tcscat_s(sImportXML, MAX_PATH, ffd.cFileName);
						// Import XML
						if(importXMLHistory(sImportXML, hContact) == 0) {
							// Abort import
							goto END_IMPORT;
						}
					} while(FindNextFile(hFind, &ffd) != 0);
					FindClose(hFind);
				}
			}
			// Update progress bar
			nImportedContactNum = nImportedContactNum + 1;
			SET_PROGRESS(100 * nImportedContactNum / nNumberOfContacts);
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
END_IMPORT:
	AddMessage("");
	AddMessage(LPGEN("%d message events added."), nImportedMessagesCount);
	AddMessage(LPGEN("%d duplicated message events skipped."), nDupes);
	AddMessage(LPGEN("%d filtered message events skipped."), nFiltered);
	AddMessage(LPGEN("%d chat log message events skipped."), nSkippedChat);
	AddMessage(LPGEN("%d message events failed to import."), nFailed);
}
