#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>

static struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return (tp2.tv_sec + (tp2.tv_usec*1.0e-6)) - 
		(tp1.tv_sec + (tp1.tv_usec*1.0e-6));
}



/* hook into the front of the list */
#define DLIST_ADD(list, p) \
do { \
        if (!(list)) { \
		(list) = (p); \
		(p)->next = (p)->prev = NULL; \
	} else { \
		(list)->prev = (p); \
		(p)->next = (list); \
		(p)->prev = NULL; \
		(list) = (p); \
	}\
} while (0)

/* remove an element from a list - element doesn't have to be in list. */
#define DLIST_REMOVE(list, p) \
do { \
	if ((p) == (list)) { \
		(list) = (p)->next; \
		if (list) (list)->prev = NULL; \
	} else { \
		if ((p)->prev) (p)->prev->next = (p)->next; \
		if ((p)->next) (p)->next->prev = (p)->prev; \
	} \
	if ((p) != (list)) (p)->next = (p)->prev = NULL; \
} while (0)

/* promote an element to the top of the list */
#define DLIST_PROMOTE(list, p) \
do { \
          DLIST_REMOVE(list, p); \
          DLIST_ADD(list, p); \
} while (0)


struct foo {
	struct foo *next, *prev;
	const char *name;
};

static struct foo *list;

static struct foo *findit(const char *name)
{
	struct foo *f;
	for (f=list;f;f=f->next) {
		if (strcmp(name, f->name) == 0) {
//			if (f != list) DLIST_PROMOTE(list, f);
			return f;
		}
	}
	return NULL;
}

int main(void)
{
	int i;
	
	for (i=0;i<20;i++) {
		struct foo *f;
		f = malloc(sizeof(*f));
		asprintf(&f->name, "%u%s", i, "foobar");
		DLIST_ADD(list, f);
	}


	for (i=0;i<20;i++) {
		char *name;
		int j;
		asprintf(&name, "%u%s", i, "foobar");
		start_timer();
		for (j=0;j<100000;j++) {
			findit(name);
		}
		printf("%s: %.2f usec/op\n", name, 1.0e6*end_timer()/j);
		free(name);
	}

	return 0;
}
