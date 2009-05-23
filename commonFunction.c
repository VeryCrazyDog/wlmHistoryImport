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
 * FileName: commonFunction.c
 * Description: This file contains all utility funcions for Miranda IM function
 * related operation. All functions here, except "utf8ToWCHAR", are originate
 * from Import plugin for Miranda IM without modification
 *
 */

#include "WLMHistoryImport.h"
#include "resource.h"

////////////////////////////////////////////////////////////////////////////////
//                Functions from Import plugin for Miranda IM                 //
////////////////////////////////////////////////////////////////////////////////

// These variables are used in function isDuplicateEvent
static DWORD dwPreviousTimeStamp = -1;
static HANDLE hPreviousContact = INVALID_HANDLE_VALUE;
static HANDLE hPreviousDbEvent = NULL;

BOOL isProtocolLoaded(char* pszProtocolName)
{
	return CallService(MS_PROTO_ISPROTOCOLLOADED, 0, (LPARAM)pszProtocolName) ? TRUE : FALSE;
}

// Returns TRUE if the event already exist in the database
BOOL isDuplicateEvent(HANDLE hContact, DBEVENTINFO dbei)
{
	HANDLE hExistingDbEvent;
	DBEVENTINFO dbeiExisting;
	DWORD dwFirstEventTimeStamp;
	DWORD dwLastEventTimeStamp;
	BOOL BadDb = FALSE;

	if (!(hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDFIRST, (WPARAM)hContact, 0)))
		return FALSE;

	ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
	dbeiExisting.cbSize = sizeof(dbeiExisting);
	dbeiExisting.cbBlob = 0;
	CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);
	dwFirstEventTimeStamp = dbeiExisting.timestamp;

	if (!(hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0)))
		return FALSE;

	ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
	dbeiExisting.cbSize = sizeof(dbeiExisting);
	dbeiExisting.cbBlob = 0;
	CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);
	dwLastEventTimeStamp = dbeiExisting.timestamp;

	// check for DB consistency
	if (dbeiExisting.timestamp < dwFirstEventTimeStamp) {
		dwLastEventTimeStamp = dwFirstEventTimeStamp;
		dwFirstEventTimeStamp = dbeiExisting.timestamp;
		BadDb = TRUE;
	}

	// compare with first timestamp
	if (dbei.timestamp < dwFirstEventTimeStamp)
		return FALSE;

	// compare with last timestamp
	if (dbei.timestamp > dwLastEventTimeStamp)
		return FALSE;

	if (hContact != hPreviousContact || BadDb) {
		hPreviousContact = hContact;
		// remember last event
		dwPreviousTimeStamp = dwLastEventTimeStamp;
		hPreviousDbEvent = hExistingDbEvent;
	}
	else {
		// fix for equal timestamps
		if (dbei.timestamp == dwPreviousTimeStamp) {
			// use last history msg
			//	dwPreviousTimeStamp = dwLastEventTimeStamp;
			//	hPreviousDbEvent = hExistingDbEvent;

			// last history msg timestamp & handle
			HANDLE hLastDbEvent = hExistingDbEvent;

			// find event with another timestamp
			hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hPreviousDbEvent, 0);
			while (hExistingDbEvent != NULL) {
				ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
				dbeiExisting.cbSize = sizeof(dbeiExisting);
				dbeiExisting.cbBlob = 0;
				CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);

				if (dbeiExisting.timestamp != dwPreviousTimeStamp)
					break;

				hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hExistingDbEvent, 0);
			}

			if (hExistingDbEvent != NULL) 			{
				// use found msg
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				hPreviousDbEvent = hExistingDbEvent;
			}
			else {
				// use last msg
				dwPreviousTimeStamp = dwLastEventTimeStamp;
				hPreviousDbEvent = hLastDbEvent;
				hExistingDbEvent = hLastDbEvent;
			}
		}
		else // use previous saved
			hExistingDbEvent = hPreviousDbEvent;
	}

	if (dbei.timestamp <= dwPreviousTimeStamp) { 	// look back
		while (hExistingDbEvent != NULL) {
			ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
			dbeiExisting.cbSize = sizeof(dbeiExisting);
			dbeiExisting.cbBlob = 0;
			CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);

			if (dbei.timestamp > dbeiExisting.timestamp) {
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return FALSE;
			}

			// Compare event with import candidate
			if ( dbei.timestamp == dbeiExisting.timestamp && 
				  dbei.eventType == dbeiExisting.eventType && 
				  dbei.cbBlob == dbeiExisting.cbBlob ) {
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return TRUE;
			}

			// Get previous event in chain
			hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hExistingDbEvent, 0);
		}
	}
	else { 	// look forward
		while (hExistingDbEvent != NULL) {
			ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
			dbeiExisting.cbSize = sizeof(dbeiExisting);
			dbeiExisting.cbBlob = 0;
			CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);

			if (dbei.timestamp < dbeiExisting.timestamp) {
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return FALSE;
			}

			// Compare event with import candidate
			if ( dbei.timestamp == dbeiExisting.timestamp && 
				  dbei.eventType == dbeiExisting.eventType && 
				  dbei.cbBlob == dbeiExisting.cbBlob ) {
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return TRUE;
			}

			// Get next event in chain
			hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDNEXT, (WPARAM)hExistingDbEvent, 0);
		}
	}
	return FALSE;
}

void utf8ToAnsi(const char *szIn, char *szOut, int cchOut)
{
	if (GetVersion() != 4 || !(GetVersion() & 0x80000000)) {
		WCHAR *wszTemp;
		int inlen;

		inlen = strlen(szIn);
		wszTemp = (WCHAR *)malloc(sizeof(WCHAR) * (inlen + 1));
		MultiByteToWideChar(CP_UTF8, 0, szIn, -1, wszTemp, inlen + 1);
		WideCharToMultiByte(CP_ACP, 0, wszTemp, -1, szOut, cchOut, NULL, NULL);
		free(wszTemp);
	}
	else {  /* this hand-rolled version isn't as good because it doesn't do DBCS */
		for (; *szIn && cchOut > 1; szIn++) {
			if (*szIn >= 0) {
				*szOut++ = *szIn;
				cchOut--;
			}
			else {
				unsigned short wideChar;

				if ((unsigned char)*szIn >= 0xe0) {
					wideChar = ((szIn[0]&0x0f) << 12)
					         | ((szIn[1]&0x3f) << 6)
							  | (szIn[2]&0x3f);
					szIn += 2;
				}
				else {
					wideChar = ((szIn[0]&0x1f) << 6)
							  | (szIn[1]&0x3f);
					szIn++;
				}
				if (wideChar >= 0x100) *szOut++ = '?';
				else *szOut++ = (char)wideChar;
				cchOut--;
		}	}

		*szOut = '\0';
	}
}

////////////////////////////////////////////////////////////////////////////////
//        Functions from Windows Live Messenger History Import Plugin         //
////////////////////////////////////////////////////////////////////////////////

void utf8ToWCHAR(const char *inString, WCHAR *outString, int outStringSize)
{
	MultiByteToWideChar(CP_UTF8, 0, inString, -1, outString, outStringSize);
	outString[outStringSize - 1] = '\0';
}
