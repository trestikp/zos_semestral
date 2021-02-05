#include "general_functions.h"

int add_lifo(link **head, void *data) {
	link *new = malloc(sizeof(link));
	return_error_on_condition(!new, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	new->data = data;
	new->next = *head;
	*head = new;

	return 0;
}

int add_fifo(link **head, void *data) {
	link *new = malloc(sizeof(link)), *ptr = *head;
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

void free_list(link *head) {
	link *ptr = head;

	while(ptr) {
		head = ptr;
		ptr = ptr->next;
		free(head->data);
		free(head);
	}
}
