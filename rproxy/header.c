#include "rproxy.h"

/* use a linked list of headers */
struct header {
	struct header *next, *prev;
	char *label;
	char *content;
};

static struct header *headers;
static int num_headers;

void header_add(char *label, char *content)
{
	struct header *h;
	h = (struct header *)xmalloc(sizeof(*h));
	
	h->label = xstrdup(label);
	h->content = xstrdup(content);
	
	DLIST_ADD(headers, h);

#if DEBUG
	printf("[%d] %s: %s\n", num_headers, label, content);
#endif

	num_headers++;
}

/* read in the headers */
void header_load(FILE *f)
{
	char line[1024];

	header_clear();

	while (1) {
		char *p;

		if (!fgets_strip(line, sizeof(line)-1, f)) {
			printf("Failed to read header %d\n", num_headers);
			exit_cleanup(1);
		}

		/* it may be the end of the headers */
		if (line[0] == 0) break;

		p = strchr(line,':');
		if (!p) {
			printf("no colon in header?!? [%s]\n", line);
			exit_cleanup(1);
		}

		*p++ = 0;
		while (*p && isspace(*p)) p++;

		header_add(line, p);
	}
}

/* find a particular header - internal function */
static struct header *header_find(char *label)
{
	struct header *h;

	for (h=headers; h; h=h->next) {
		if (strcasecmp(label,h->label) == 0) {
			return h;
		}
	}
	return NULL;
}


/* find a particular header - return the content */
char *header_content(char *label)
{
	struct header *h = header_find(label);

	if (h) return h->content;

	return NULL;
}

/* return an integer header or zero */
int header_ival(char *label)
{
	char *c = header_content(label);
	if (!c) return -1;
	return atoi(c);
}

/* send all the headers to a stream */
void header_send(FILE *f)
{
	struct header *h;

	for (h=headers; h; h=h->next) {
		fprintf(f,"%s: %s\r\n", h->label, h->content);
	}	
	fprintf(f,"\r\n");
}

void header_clear(void)
{
	struct header *h, *next;

	for (h=headers; h; h=next) {
		next = h->next;
		free(h->label);
		free(h->content);
		DLIST_REMOVE(headers, h);
		free(h);
	}	
	num_headers = 0;
}

void header_remove(char *label)
{
	struct header *h = header_find(label);

	if (!h) return;

	free(h->label);
	free(h->content);
	DLIST_REMOVE(headers, h);
	free(h);
	num_headers--;
}

/* remove a particular element from a header list */
void header_remove_list(char *label, char *content)
{
	struct header *h = header_find(label);
	char *p;

	if (!h) return;

	p = strstr(h->content, content);
	if (!p) return;

	if (!strchr(h->content,',')) {
		header_remove(label);
		return;
	}

	string_sub(h->content, content, "");
	string_sub(h->content, ", ,", ",");
	trim_string(h->content," ", " ");
	trim_string(h->content,",", ",");
}


/* add a element to a header list */
void header_add_list(char *label, char *content)
{
	struct header *h = header_find(label);
	char *p;

	if (!h) {
		header_add(label, content);
		return;
	}

	p = (char *)xmalloc(strlen(h->content) + strlen(content) + 3);
	strcpy(p, h->content);
	strcat(p, ", ");
	strcat(p, content);

	free(h->content);
	h->content = p;
}

