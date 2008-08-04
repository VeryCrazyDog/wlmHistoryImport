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
