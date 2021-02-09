#include "general_functions.h"

/**
	Adds new link with @data as first element in list @head
*/
int add_lifo(link **head, void *data) {
	link *new = calloc(1, sizeof(link));
	return_error_on_condition(!new, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	new->data = data;
	new->next = *head;
	*head = new;

	return 0;
}

/**
	Adds new link with @data at the end of list @head
*/
int add_fifo(link **head, void *data) {
	link *new = calloc(1, sizeof(link)), *ptr = *head;
	return_error_on_condition(!new, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	new->data = data;
	new->next = NULL;

	if(!*head) {
		*head = new;
		ptr = *head;
	}

	while(ptr->next) ptr = ptr->next;

	ptr->next = new;

	return 0;
}
