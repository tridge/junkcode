/*
  a mmap based memory allocator

  tridge@samba.org, July 2007  
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>

#if (__GNUC__ >= 3)
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

#define PAGE_SIZE   4096
#define PAGE_MASK   (~(intptr_t)(PAGE_SIZE-1))
#define NUM_BUCKETS    7
#define BASE_BUCKET   40
#define FREE_PAGE_LIMIT 2
#define LARGEST_BUCKET (BASE_BUCKET*(1<<(NUM_BUCKETS-1)))
#define MAX_ELEMENTS (PAGE_SIZE/BASE_BUCKET)

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

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


struct page_header {
	uint32_t num_pages;
	uint16_t num_elements;
	uint16_t elements_used;
	struct page_header *next, *prev;
	struct bucket_state *bucket;
	uint32_t used[(MAX_ELEMENTS+31)/32];
};


struct bucket_state {
	uint32_t alloc_limit;
	uint32_t num_free_pages;
	struct page_header *page_list;
	struct page_header *full_list;
	struct page_header *empty_list;
};

struct alloc_state {
	struct bucket_state buckets[NUM_BUCKETS];
	bool initialised;
};

static struct alloc_state state;

/*
  initialise the allocator
 */
static void alloc_initialise(void)
{
	int i;
	state.buckets[0].alloc_limit = BASE_BUCKET;
	for (i=1;i<NUM_BUCKETS;i++) {
		state.buckets[i].alloc_limit = state.buckets[i-1].alloc_limit * 2;
	}
	state.initialised = true;
}


/*
  large allocation function - use mmap per allocation
 */
static void *alloc_large(size_t size)
{
	uint32_t num_pages = (size+sizeof(uint32_t)+(PAGE_SIZE-1))/PAGE_SIZE;
	void *p;
	uint32_t *nump;

	p = mmap(0, num_pages*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (p == (void *)-1) {
		return NULL;
	}

	nump = (uint32_t *)p;
	*nump = num_pages;

	return (void *)(nump+1);
}


/*
  large free function
 */
static void alloc_large_free(void *ptr)
{
	uint32_t *nump = ((uint32_t *)ptr)-1;
	munmap(ptr, (*nump)*PAGE_SIZE);
}

/*
  refill a bucket
 */
static void alloc_refill_bucket(struct bucket_state *bs)
{
	void *p;
	struct page_header *ph;

	ph = bs->empty_list;
	if (ph != NULL) {
		DLIST_REMOVE(bs->empty_list, ph);
		DLIST_ADD(bs->page_list, ph);
		return;
	}

	p = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (p == (void *)-1) {
		return;
	}

	ph = (struct page_header *)p;
	ph->bucket = bs;
	ph->num_elements = (PAGE_SIZE-sizeof(struct page_header)) / bs->alloc_limit;

	DLIST_ADD(bs->page_list, ph);

	bs->num_free_pages++;
//	fprintf(stderr, "added page %p\n", ph);
}


/*
  find the first free element in a bitmap. This assumes there is a bit
  to be found! If not, you will get weird results
 */
static inline int alloc_find_free_element(struct page_header *ph)
{
	int i, j;
	uint32_t *used = ph->used;
	uint32_t n = (ph->num_elements+31)/32;
	for (i=0;i<n;i++) {
		if (used[i] != 0xFFFFFFFF) break;
	}
	j = ffs(~used[i]) - 1;
	i = (i*32) + j;
	return i;
}

/*
  allocate some memory
 */
static void *alloc_malloc(size_t size)
{
	int i;
	struct bucket_state *bs;
	struct page_header *ph;

	if (unlikely(state.initialised == false)) {
		alloc_initialise();
	}

	/* is it a large allocation? */
	if (unlikely(size > LARGEST_BUCKET)) {
		return alloc_large(size);
	}

	/* find the right bucket */
	for (i=0;i<NUM_BUCKETS;i++) {
		if (state.buckets[i].alloc_limit >= size) {
			break;
		}
	}

	bs = &state.buckets[i];

	/* it might be empty */
	if (unlikely(bs->page_list == NULL)) {
		alloc_refill_bucket(bs);
		if (unlikely(bs->page_list == NULL)) {
			return NULL;
		}
	}

	/* take the first one */
	ph = bs->page_list;
	i = alloc_find_free_element(ph);

	if (ph->elements_used == 0) {
		bs->num_free_pages--;
//		fprintf(stderr,"free pages for bucket %d = %d\n", 
//			bs->alloc_limit, bs->num_free_pages);
	}
	ph->elements_used++;

	ph->used[i/32] |= 1<<(i%32);
	if (ph->elements_used == ph->num_elements) {
		DLIST_REMOVE(bs->page_list, ph);
		DLIST_ADD(bs->full_list, ph);
	}

	return (void *)((i*bs->alloc_limit)+(char *)(ph+1));
}


/*
  release all completely free pages in this bucket
 */
static void alloc_vacuum_bucket(struct bucket_state *bs)
{
	struct page_header *ph;

//	fprintf(stderr,"vacuum bucket %d\n", bs->alloc_limit);

	while ((ph = bs->empty_list)) {
		DLIST_REMOVE(bs->empty_list, ph);
		bs->num_free_pages--;
		munmap((void *)ph, PAGE_SIZE);
	}
}

/*
  free some memory
 */
static void alloc_free(void *ptr)
{
	struct page_header *ph = (struct page_header *)(PAGE_MASK & (intptr_t)ptr);
	struct bucket_state *bs;
	int i;

	if (unlikely(ptr == NULL)) {
		return;
	}

	if (ph->num_pages != 0) {
		alloc_large_free(ptr);
		return;
	}

	bs = ph->bucket;
	
	i = ((((intptr_t)ptr) - (intptr_t)ph) - sizeof(struct page_header)) / bs->alloc_limit;

	ph->used[i/32] &= ~(1<<(i%32));

	if (unlikely(ph->elements_used == ph->num_elements)) {
		DLIST_REMOVE(bs->full_list, ph);
		DLIST_ADD(bs->page_list, ph);
	}

	ph->elements_used--;

	if (ph->elements_used == 0) {
		DLIST_REMOVE(bs->page_list, ph);
		DLIST_ADD(bs->empty_list, ph);
		bs->num_free_pages++;
//		fprintf(stderr,"free pages for bucket %d = %d\n", 
//			bs->alloc_limit, bs->num_free_pages);
		if (bs->num_free_pages == FREE_PAGE_LIMIT) {
			alloc_vacuum_bucket(bs);
		}
	}
}


/*
  realloc for large blocks
 */
void *alloc_realloc_large(void *ptr, size_t size)
{
	void *ptr2;
	uint32_t *nump = ((uint32_t *)ptr)-1;

	if ((*nump)*PAGE_SIZE >= (size + sizeof(uint32_t))) {
		return ptr;
	}

	ptr2 = alloc_malloc(size);
	if (ptr2 == NULL) {
		return NULL;
	}

	memcpy(ptr2, ptr, MIN(size, ((*nump)*PAGE_SIZE)-sizeof(uint32_t)));
	alloc_large_free(ptr);
	return ptr2;
}



/*
  realloc
 */
static void *alloc_realloc(void *ptr, size_t size)
{
	struct page_header *ph = (struct page_header *)(PAGE_MASK & (intptr_t)ptr);
	struct bucket_state *bs;
	void *ptr2;

	if (size == 0) {
		alloc_free(ptr);
		return NULL;
	}

	if (ptr == NULL) {
		return alloc_malloc(size);
	}

	if (ph->num_pages != 0) {
		return alloc_realloc_large(ptr, size);
	}	

	bs = ph->bucket;
	if (bs->alloc_limit >= size) {
		return ptr;
	}

	ptr2 = alloc_malloc(size);
	if (ptr2 == NULL) {
		return NULL;
	}

	memcpy(ptr2, ptr, MIN(bs->alloc_limit, size));
	alloc_free(ptr);
	return ptr2;
}


/*
  calloc
 */
static void *alloc_calloc(size_t nmemb, size_t size)
{
	void *ptr;
	if (unlikely(size > LARGEST_BUCKET)) {
		return alloc_large(nmemb*size);
	}
	ptr = alloc_malloc(nmemb*size);
	if (ptr) {
		memset(ptr, 0, nmemb*size);
	}
	return ptr;
}

#if 1
void *malloc(size_t size)
{
	return alloc_malloc(size);
}

void *calloc(size_t nmemb, size_t size)
{
	return alloc_calloc(nmemb, size);
}

void free(void *ptr)
{
	alloc_free(ptr);
}

void *realloc(void *ptr, size_t size)
{
	return alloc_realloc(ptr, size);
}
#endif
