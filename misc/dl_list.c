/*
 * Doubly-linked list implementation with position marker
 * MD 2211 <md2211@users.sourceforge.net>, 2007
 */

#include <stdlib.h>
#include "u_mem.h"
#include "dl_list.h"

dl_list *dl_init() {
	dl_list *list = d_malloc(sizeof(dl_list));
	list->first = NULL;
	list->last = NULL;
	list->current = NULL;
	list->size = 0;
	return list;
}

void dl_add(dl_list *list, void *data) {
	dl_item *item;
	item = d_malloc(sizeof(dl_item));
	item->data = data;
	item->prev = list->last;
	item->next = NULL;

	if (list->last != NULL) {
		list->last->next = item;
	}
	if (list->first == NULL) {
		list->first = item;
		list->current = item;
	}
	list->last = item;
	list->size++;
}

void dl_remove(dl_list *list, dl_item *item) {
	if (list->current == item) list->current = item->prev;

	if (list->first == item) list->first = item->next;
	else item->prev->next = item->next;

	if (list->last == item) list->last = item->prev;
	else item->next->prev = item->prev;

	d_free(item);
	list->size--;
}

int dl_is_empty(dl_list const *list) {
	return (list->first == NULL);
}

int dl_size(dl_list const *list) {
    return list->size;
}

int dl_forward(dl_list *list) {
	if (!dl_is_empty(list) && list->current->next != NULL) {
		list->current = list->current->next;
		return 1;
	}
	else return 0;
}

int dl_backward(dl_list *list) {
	if (!dl_is_empty(list) && list->current->prev != NULL) {
		list->current = list->current->prev;
		return 1;
	}
	else return 0;
}
