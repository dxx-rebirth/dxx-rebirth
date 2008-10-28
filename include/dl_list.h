/*
 * Doubly-linked list implementation
 * MD 2211 <md2211@users.sourceforge.net>, 2007
 */

#ifndef __DL_LIST__
#define __DL_LIST__

struct dl_list_elem {
	void *data;
	struct dl_list_elem *prev;
	struct dl_list_elem *next;
};

typedef struct dl_list_elem dl_item;

typedef struct {
	struct dl_list_elem *first;
	struct dl_list_elem *last;
	struct dl_list_elem *current;
	unsigned int         size;
} dl_list;

dl_list *dl_init();
void dl_add(dl_list *, void *);
void dl_remove(dl_list *, dl_item *);
int dl_is_empty(dl_list const *);
int dl_size(dl_list const *);
int dl_forward(dl_list *);
int dl_backward(dl_list *);

#endif
