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
 * FileName: commonFunction.cpp
 * Description: This file contains all utility funcions for Miranda IM 
 * related operation.
 *
 */

#include "WLMHistoryImport.h"

#include <m_database.h>
#include <m_protosvc.h>

////////////////////////////////////////////////////////////////////////////////
//                Functions from Import plugin for Miranda IM                 //
////////////////////////////////////////////////////////////////////////////////

// Returns TRUE if the event already exist in the database
BOOL isDuplicateEvent(HANDLE hContact, DBEVENTINFO dbei)
{
	static DWORD dwPreviousTimeStamp = -1;
	static HANDLE hPreviousContact = INVALID_HANDLE_VALUE;
	static HANDLE hPreviousDbEvent = NULL;

	HANDLE hExistingDbEvent;
	DWORD dwEventTimeStamp;
	DBEVENTINFO dbeiExisting;

	// get last event
	if (!(hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0)))
		return FALSE;

	ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
	dbeiExisting.cbSize = sizeof(dbeiExisting);
	CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);
	dwEventTimeStamp = dbeiExisting.timestamp;

	// compare with last timestamp
	if (dbei.timestamp > dwEventTimeStamp)
	{
		// remember event
		hPreviousDbEvent = hExistingDbEvent;
		dwPreviousTimeStamp = dwEventTimeStamp;
		return FALSE;
	}

	if (hContact != hPreviousContact)
	{
		hPreviousContact = hContact;
		// remember event
		hPreviousDbEvent = hExistingDbEvent;
		dwPreviousTimeStamp = dwEventTimeStamp;

   		// get first event
		if (!(hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDFIRST, (WPARAM)hContact, 0)))
			return FALSE;

		ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
		dbeiExisting.cbSize = sizeof(dbeiExisting);
		CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);
		dwEventTimeStamp = dbeiExisting.timestamp;

		// compare with first timestamp
		if (dbei.timestamp <= dwEventTimeStamp)
		{
		    // remember event
			dwPreviousTimeStamp = dwEventTimeStamp;
			hPreviousDbEvent = hExistingDbEvent;

			if ( dbei.timestamp != dwEventTimeStamp )
				return FALSE;
		}

	}
	// check for equal timestamps
	if (dbei.timestamp == dwPreviousTimeStamp)
	{
		ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
		dbeiExisting.cbSize = sizeof(dbeiExisting);
		CallService(MS_DB_EVENT_GET, (WPARAM)hPreviousDbEvent, (LPARAM)&dbeiExisting);

		if ((dbei.timestamp == dbeiExisting.timestamp) &&
			(dbei.eventType == dbeiExisting.eventType) &&
			(dbei.cbBlob == dbeiExisting.cbBlob) &&
			((dbei.flags&DBEF_SENT) == (dbeiExisting.flags&DBEF_SENT)))
    		return TRUE;

		// find event with another timestamp
		hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDNEXT, (WPARAM)hPreviousDbEvent, 0);
		while (hExistingDbEvent != NULL)
		{
			ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
			dbeiExisting.cbSize = sizeof(dbeiExisting);
			CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);

			if (dbeiExisting.timestamp != dwPreviousTimeStamp)
			{
				// use found event
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				break;
			}

			hPreviousDbEvent = hExistingDbEvent;
			hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDNEXT, (WPARAM)hExistingDbEvent, 0);
		}
	}

	hExistingDbEvent = hPreviousDbEvent;

	if (dbei.timestamp <= dwPreviousTimeStamp)
	{
		// look back
		while (hExistingDbEvent != NULL)
		{
			ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
			dbeiExisting.cbSize = sizeof(dbeiExisting);
			CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);

			if (dbei.timestamp > dbeiExisting.timestamp)
			{
			    // remember event
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return FALSE;
			}

			// Compare event with import candidate
			if ((dbei.timestamp == dbeiExisting.timestamp) &&
				(dbei.eventType == dbeiExisting.eventType) &&
				(dbei.cbBlob == dbeiExisting.cbBlob) &&
				((dbei.flags&DBEF_SENT) == (dbeiExisting.flags&DBEF_SENT)))
			{
				// remember event
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return TRUE;
			}

			// Get previous event in chain
			hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hExistingDbEvent, 0);
		}

    }
    else
    {
		// look forward
		while (hExistingDbEvent != NULL)
		{
			ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
			dbeiExisting.cbSize = sizeof(dbeiExisting);
			CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);

			if (dbei.timestamp < dbeiExisting.timestamp)
			{
			    // remember event
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return FALSE;
			}

			// Compare event with import candidate
			if ((dbei.timestamp == dbeiExisting.timestamp) &&
				(dbei.eventType == dbeiExisting.eventType) &&
				(dbei.cbBlob == dbeiExisting.cbBlob) &&
				((dbei.flags&DBEF_SENT) == (dbeiExisting.flags&DBEF_SENT)))
			{
				// remember event
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return TRUE;
			}

			// Get next event in chain
			hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDNEXT, (WPARAM)hExistingDbEvent, 0);
		}

	}
	// reset last event
	hPreviousContact = INVALID_HANDLE_VALUE;
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////
////        Functions from Windows Live Messenger History Import Plugin         //
//////////////////////////////////////////////////////////////////////////////////

// This returns the local UTC time offset in seconds
// Return: Offset between UTC+0 and local time in seconds
int getLocalUTCOffset() {
	TIME_ZONE_INFORMATION TimeZoneInformation;

	GetTimeZoneInformation(&TimeZoneInformation);
	return -TimeZoneInformation.Bias * 60;
}

// The returned pointer need to be free by calling "mir_free"
TCHAR *replaceVariable(TCHAR *toReplace) {
	REPLACEVARSDATA dat = {0};
	dat.cbSize = sizeof(dat);
	dat.dwFlags = RVF_TCHAR;

	return (TCHAR*)CallService(MS_UTILS_REPLACEVARS, (WPARAM)toReplace, (LPARAM)&dat);
}

// This returns the number of contact in current profile with the given unique physical account name
// Return: Number of contact in current profile with the given unique physical account name
unsigned int getNumOfContact(const char *accModuleName) {
	HANDLE hContact = NULL;
	unsigned int nContactCount = 0;

	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		char *accBaseName = (char *)CallService(MS_PROTO_GETCONTACTBASEACCOUNT, (WPARAM)hContact, 0);
		if(accBaseName != NULL && strcmp(accBaseName, accModuleName) == 0) {
			nContactCount = nContactCount + 1;
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	return nContactCount;
}

// This returns the ID of a contact
// Param: [out]sID - ID of the queried contact
//        [in]idLen - Length of the variable sID
//        [in]hContact - Handle to the queried contact
// Return: TRUE when the email is obtained successfully, otherwise FALSE
BOOL getUserID(char *sID, const size_t idLen, const HANDLE hContact) {
	BOOL result;
	char *accBaseName;
	char *pszUniqueSetting;

	accBaseName = (char *)CallService(MS_PROTO_GETCONTACTBASEACCOUNT, (WPARAM)hContact, 0);
	pszUniqueSetting = (char *)CallProtoService(accBaseName, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
	if (pszUniqueSetting && (INT_PTR)pszUniqueSetting != CALLSERVICE_NOTFOUND) {
		DBVARIANT dbv;
		DBCONTACTGETSETTING sVal;

		dbv.pszVal = sID;
		dbv.cchVal = (WORD)idLen;
		dbv.type = DBVT_ASCIIZ;
		sVal.pValue = &dbv;
		sVal.szModule = accBaseName;
		sVal.szSetting = pszUniqueSetting;
		result = (CallService(MS_DB_CONTACT_GETSETTINGSTATIC, (WPARAM)hContact, (LPARAM)&sVal) == 0);
	}
	else {
		result = FALSE;
	}
	return result;
}
