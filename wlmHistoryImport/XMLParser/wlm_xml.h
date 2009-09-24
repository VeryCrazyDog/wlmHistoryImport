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

	This file is modified based on "m_xml.h" in Miranda IM source code.

 */

#include <tchar.h>

DECLARE_HANDLE(HXML);

typedef struct
{
	int cbSize;

	HXML    ( *createNode )( LPCTSTR name, LPCTSTR text, char bIsDeclaration );
	void    ( *destroyNode )( HXML node );

	HXML    ( *parseString )( LPCTSTR string, int* datalen, LPCTSTR tag );
	LPTSTR  ( *toString )( HXML node, int* datalen );

	HXML    ( *addChild )( HXML parent, LPCTSTR name, LPCTSTR text );
	void    ( *addChild2 )( HXML child, HXML parent );
	HXML    ( *copyNode )( HXML parent);
	HXML    ( *getChild )( HXML parent, int number );
	int     ( *getChildCount )( HXML );
	HXML    ( *getChildByAttrValue )( HXML parent, LPCTSTR name, LPCTSTR attrName, LPCTSTR attrValue );
	HXML    ( *getFirstChild )( HXML parent );
	HXML    ( *getNthChild )( HXML parent, LPCTSTR name, int n );
	HXML    ( *getNextChild )( HXML parent, LPCTSTR name, int *i );
	HXML    ( *getChildByPath )( HXML parent, LPCTSTR path, char createNodeIfMissing );
	HXML    ( *getNextNode )( HXML node );
	LPCTSTR ( *getName )( HXML );
	HXML    ( *getParent )( HXML );
	LPCTSTR ( *getText )( HXML );
	void    ( *setText )( HXML, LPCTSTR );

	LPCTSTR ( *getAttr )( HXML, int n );
	LPCTSTR ( *getAttrName )( HXML, int n );
	LPCTSTR ( *getAttrValue )( HXML, LPCTSTR attrName );
	int     ( *getAttrCount )( HXML );
	void    ( *addAttr )( HXML, LPCTSTR attrName, LPCTSTR attrValue );
	void    ( *addAttrInt )( HXML, LPCTSTR attrName, int attrValue );

	void    ( *freeMem )( void* );
}
	XML_API;

extern XML_API xi;

INT_PTR GetXmlApi( WPARAM, LPARAM lParam );

__forceinline int wlm_getXI( XML_API* dest )
{
	dest->cbSize = sizeof(*dest);
	return GetXmlApi(NULL, (LPARAM)dest);
}
