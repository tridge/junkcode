#define DLIST_ADD(list, p) dlist_add((void *)list, (void *)p, \
                                     (void **)&list->next, \ 
				     (void **)&list->prev, \
                                     (void **)&p->next, \
				     (void **)&p->prev)

