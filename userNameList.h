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

/*
 *
 * FileName: userNameList.h
 * Description: This file contains implementation of a string list, mainly
 * used for storing different user names for the currect profile owner.
 *
 */

#ifndef LIST_H
#define LIST_H

#define LIST_SIZE 16
#define MAX_USER_NAME 32

/*
 * Note:
 *
 * TCHAR (*fe)[MAX_USER_NAME];
 * It is a pointer to an array of single dimension with length MAX_USER_NAME
 *
 * TCHAR *fe[MAX_USER_NAME];
 * It is an array of pointers to TCHAR with length MAX_USER_NAME
 *
 */

typedef struct {
	TCHAR data[LIST_SIZE][MAX_USER_NAME];
	// Pointer points to first element
	TCHAR (*fe)[MAX_USER_NAME];
	// Pointer points to last element
	TCHAR (*le)[MAX_USER_NAME];
} list;

void list_Reset(list *list) {
	list->fe = list->data;
	list->le = list->data;
}

BOOL list_Add(list *list, const TCHAR *data) {
	BOOL result;

	result = !(list->le + 1 == (list->fe + LIST_SIZE));
	if(result == TRUE) {
		list->le = list->le + 1;
		_tcscpy_s(*list->le, MAX_USER_NAME, data);
	}
	return result;
}

BOOL list_Contains(list *list, const TCHAR *data) {
	int i;
	BOOL result = FALSE;

	// No data for i = 0
	for(i = 1; list->data[i] <= *list->le ; i = i + 1) {
		if(_tcscmp(list->data[i], data) == 0) {
			result = TRUE;
			break;
		}
	}
	return result;
}

#endif
