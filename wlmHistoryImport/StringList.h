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
 * FileName: StringList.h
 * Description: This file contains implementation of a string list.
 *
 */

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

#ifndef STRING_LIST_H
#define STRING_LIST_H

#include <list>

using namespace std;

class StringList {

private:
	list<TCHAR *> cData;

public:
	StringList() {};
	~StringList() {
		clear();
	}

public:
	void addToFront(TCHAR *value) {
		TCHAR *buf;
		size_t len;

		len = _tcslen(value);
		buf = new TCHAR[len + 1];
		_tcsncpy_s(buf, len + 1, value, _TRUNCATE);
		cData.push_front(buf);
	}

	void clear() {
		list<TCHAR *>::iterator iter;

		iter = cData.begin();
		while(iter != cData.end()) {
			delete *iter;
			iter++;
		}
		cData.clear();
	}

	BOOL contain(TCHAR *value) {
		BOOL result;
		list<TCHAR *>::iterator iter;

		result = FALSE;
		iter = cData.begin();
		while(iter != cData.end() && result == FALSE) {
			if(_tcscmp(*iter, value) == 0) {
				result = TRUE;
			}
			iter++;
		}
		return result;
	}

};

#endif
