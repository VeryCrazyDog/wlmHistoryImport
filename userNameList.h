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
			result = 1;
			break;
		}
	}
	return result;
}

#endif
