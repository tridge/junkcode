/*
  a mmap based memory allocator

  tridge@samba.org, July 2007  
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
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
#define NUM_BUCKETS    7
#define BASE_BUCKET   40
#define FREE_PAGE_LIMIT 2
#define LARGEST_BUCKET (BASE_BUCKET*(1<<(NUM_BUCKETS-1)))

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
	struct page_header *next, *prev;
	uint32_t bytes_used;
	struct bucket_state *bucket;
};


struct block_header {
	struct page_header *page;
	struct block_header *next, *prev;
	uint32_t num_pages;
};

struct bucket_state {
	uint32_t alloc_limit;
	uint32_t num_free_pages;
	struct block_header *freelist;
	struct page_header *page_list;
};

struct alloc_state {
	struct bucket_state buckets[NUM_BUCKETS];
	bool initialised;
};

static struct alloc_state state;

/*
  fatal error
 */
static void alloc_fatal(const char *msg)
{
	write(2, msg, strlen(msg));
	abort();
}

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
	uint32_t num_pages = (size+sizeof(struct block_header)+(PAGE_SIZE-1))/PAGE_SIZE;
	void *p;
	struct block_header *bh;

	p = mmap(0, num_pages*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (p == (void *)-1) {
		return NULL;
	}

	bh = (struct block_header *)p;
	bh->page = NULL;
	bh->num_pages = num_pages;

	return (void *)(bh+1);
}


/*
  large free function
 */
static void alloc_large_free(struct block_header *bh)
{
	if (unlikely(((intptr_t)bh) & (PAGE_SIZE-1))) {
		alloc_fatal("FATAL: Free of large alloc is not page aligned\n");
	}
	munmap((void *)bh, bh->num_pages*PAGE_SIZE);
}

/*
  refill a bucket
 */
static void alloc_refill_bucket(struct bucket_state *bs)
{
	void *p;
	struct page_header *ph;
	struct block_header *bh;
	uint32_t size, bsize;

	p = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (p == (void *)-1) {
		return;
	}

	ph = (struct page_header *)p;
	ph->bucket = bs;
	ph->bytes_used = 0;
	bh = (struct block_header *)(ph+1);

	DLIST_ADD(bs->page_list, ph);

	bsize = sizeof(struct block_header)+bs->alloc_limit;

	size = PAGE_SIZE - sizeof(struct page_header);
	while (size > bsize) {
		bh->page = ph;

		DLIST_ADD(bs->freelist, bh);

		size -= bsize;
		bh = (struct block_header *)(bsize + (char *)bh);
	}	

	bs->num_free_pages++;
//	fprintf(stderr, "added page %p\n", ph);
}

/*
  allocate some memory
 */
static void *alloc_malloc(size_t size)
{
	int i;
	struct bucket_state *bs;
	struct block_header *bh;

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

	if (unlikely(i==NUM_BUCKETS)) {
		alloc_fatal("FATAL: bucket error?\n");
	}

	bs = &state.buckets[i];

	/* it might be empty */
	if (unlikely(bs->freelist == NULL)) {
		alloc_refill_bucket(bs);
		if (unlikely(bs->freelist == NULL)) {
			return NULL;
		}
	}

	/* take the top one */
	bh = bs->freelist;
	DLIST_REMOVE(bs->freelist, bh);

	if (bh->page->bytes_used == 0) {
		bs->num_free_pages--;
//		fprintf(stderr,"free pages for bucket %d = %d\n", 
//			bs->alloc_limit, bs->num_free_pages);
	}

	bh->page->bytes_used += bs->alloc_limit;

	return (void *)(bh+1);
}


/*
  release a complete free page in this bucket
 */
static void alloc_page_release(struct bucket_state *bs, struct page_header *ph)
{
	struct block_header *bh;
	uint32_t bsize, size;

	bh = (struct block_header *)(ph+1);
	bsize = sizeof(struct block_header)+bs->alloc_limit;
	size = PAGE_SIZE - sizeof(struct page_header);
	while (size > bsize) {
		DLIST_REMOVE(bs->freelist, bh);
		size -= bsize;
		bh = (struct block_header *)(bsize + (char *)bh);
	}	
	DLIST_REMOVE(bs->page_list, ph);
	bs->num_free_pages--;
	munmap((void *)ph, PAGE_SIZE);
//	fprintf(stderr, "released page %p\n", ph);
}

/*
  release all completely free pages in this bucket
 */
static void alloc_vacuum_bucket(struct bucket_state *bs)
{
	struct page_header *ph, *next;

//	fprintf(stderr,"vacuum bucket %d\n", bs->alloc_limit);

	for (ph=bs->page_list;ph;ph=next) {
		next = ph->next;
		if (ph->bytes_used == 0) {
			alloc_page_release(bs, ph);
		}
	}

}

/*
  free some memory
 */
static void alloc_free(void *ptr)
{
	struct block_header *bh = ((struct block_header *)ptr)-1;
	struct bucket_state *bs;

	if (unlikely(ptr == NULL)) {
		return;
	}

	if (bh->page == NULL) {
		alloc_large_free(bh);
		return;
	}

	bs = bh->page->bucket;

	/* put at the front of the list */
	DLIST_ADD(bs->freelist, bh);

	/* reduce usage in page */
	bh->page->bytes_used -= bs->alloc_limit;
	if (bh->page->bytes_used == 0) {
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
void *alloc_realloc_large(struct block_header *bh, size_t size)
{
	void *ptr2;

	if (bh->num_pages*PAGE_SIZE >= (size + sizeof(struct block_header))) {
		return (void *)(bh+1);
	}

	ptr2 = alloc_malloc(size);
	if (ptr2 == NULL) {
		return NULL;
	}

	memcpy(ptr2, bh+1, MIN(size, (bh->num_pages*PAGE_SIZE)-sizeof(struct block_header)));
	alloc_large_free(bh);
	return ptr2;
}



/*
  realloc
 */
static void *alloc_realloc(void *ptr, size_t size)
{
	struct block_header *bh = ((struct block_header *)ptr)-1;
	struct bucket_state *bs;
	void *ptr2;

	if (size == 0) {
		alloc_free(ptr);
		return NULL;
	}

	if (ptr == NULL) {
		return alloc_malloc(size);
	}

	if (bh->page == NULL) {
		return alloc_realloc_large(bh, size);
	}	

	bs = bh->page->bucket;
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
