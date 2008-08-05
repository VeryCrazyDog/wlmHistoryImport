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
 * FileName: tagStack.h
 * Description: This file contains implementation of an integer stack, mainly
 * used for storing parsed XML tag.
 *
 */

#ifndef STACK_H
#define STACK_H

#define STACK_SIZE 16

typedef struct {
	int data[STACK_SIZE];
	// Pointer points to buttom of stack
	int *bos;
	// Pointer points to top of stack
	int *tos;
} stack;

void stack_Init(stack *stack) {
	stack->bos = stack->data;
	stack->tos = stack->data;
}

BOOL stack_Push(stack *stack, const int data) {
	BOOL result;

	result = !(stack->tos + 1 == (stack->bos + STACK_SIZE));
	if(result == TRUE) {
		stack->tos = stack->tos + 1;
		*stack->tos = data;
	}
	return result;
}

int stack_Pop(stack *stack) {
	int result;

	if(stack->bos == stack->tos) {
		result = 0;
	}
	else {
		result = *stack->tos;
		stack->tos = stack->tos - 1;
	}
	return result;
}

int stack_Top(stack *stack) {
	int result;

	if(stack->bos == stack->tos) {
		result = 0;
	}
	else {
		result = *stack->tos;
	}
	return result;
}

#endif
