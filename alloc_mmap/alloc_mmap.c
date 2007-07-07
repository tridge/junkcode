/*
  a mmap based memory allocator

   Copyright (C) Andrew Tridgell 2007
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*

  The basic aims of this allocator are:

     - fast
     - all memory comes from anonymous mmap
     - releases memory back to the OS as much as possible
     - no per-block prefix (so extremely low overhead)
     - no safety checking at all
     - no attempt at thread safety. Threads will cause crashes

  Design:

      allocations are either 'small' or 'large'. 

      'large' allocations do a plain mmap, one per allocation. When
      you free you munmap. The 'large' allocations have a struct
      large_header prefix, so the returned pointer is not in fact on a
      page boundary.

      'small' allocations come from a bucket allocator. There are
      a set of buckets, each larger than the previous by a constant
      factor. 'small' allocations do not have any prefix header before
      the allocation.

      The meta-data for the small allocations comes at the start of
      the page. This meta-data contains a bitmap of what pieces of the
      page are currently used.
      
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>
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

#define MAX_BUCKETS     15
#define BASE_BUCKET     32
#define FREE_PAGE_LIMIT  2
#define ALLOC_ALIGNMENT 16
#define BUCKET_NEXT(prev) ((3*(prev))/2)

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#define ALIGN_SIZE(size) (((size)+(ALLOC_ALIGNMENT-1))&~(ALLOC_ALIGNMENT-1))

/* work out how many bytes aligning to ALLOC_ALIGNMENT plus at least sizeof(type)
   will cost */
#define ALIGN_COST(type) ALIGN_SIZE(sizeof(type))

/* take a pointer, and move it up by at least one sizeof(type), aligned
   to the ALLOC_ALIGNMENT */
#define ALIGN_UP(ptr, type) ((ALIGN_COST(type)+(intptr_t)(ptr)))

/* align to the start of the page, and return a type* */
#define ALIGN_DOWN_PAGE(ptr, type) (type *)((~(intptr_t)(state.page_size-1)) & (intptr_t)(ptr))

/* work out the aligned size of a page header, given the size of the used[] array */
#define PAGE_HEADER_SIZE(n) ALIGN_SIZE(offsetof(struct page_header, used) + 4*(((n)+31)/32))

/* move an element from the front of list1 to the front of list2 */
#define MOVE_LIST_FRONT(list1, list2, p) \
	do { \
		list1 = p->next;	       \
		if (list1) list1->prev = NULL; \
		p->next = list2;		\
		if (p->next) p->next->prev = p;	\
		list2 = p;			\
	} while (0)

/* move an element from the anywhere in list1 to the front of list2 */
#define MOVE_LIST(list1, list2, p) \
	do { \
		if (p == list1) { \
			MOVE_LIST_FRONT(list1, list2, p); \
		} else { \
			p->prev->next = p->next; \
			if (p->next) p->next->prev = p->prev; \
			(p)->next = list2; \
			if (p->next) (p)->next->prev = (p);	\
			p->prev = NULL; \
			list2 = (p);	\
		} \
	} while (0)

struct page_header {
	uint32_t num_pages; /* maps to num_pages in struct large_header */
	uint16_t elements_used;
	uint16_t num_elements;
	struct page_header *next, *prev;
	struct bucket_state *bucket;
	uint32_t used[1];
};

struct large_header {
	uint32_t num_pages;
};


struct bucket_state {
	uint32_t alloc_limit;
	uint16_t elements_per_page;
	struct page_header *page_list;
	struct page_header *full_list;
};

struct alloc_state {
	bool initialised;
	uint32_t page_size;
	uint32_t num_empty_pages;
	uint32_t largest_bucket;
	uint32_t max_bucket;
	struct page_header *empty_list;
	struct bucket_state buckets[MAX_BUCKETS];
};

/* static state of the allocator */
static struct alloc_state state;

/*
  initialise the allocator
 */
static void alloc_initialise(void)
{
	int i;
	state.page_size = getpagesize();
	state.buckets[0].alloc_limit = BASE_BUCKET;
	for (i=0;i<MAX_BUCKETS;i++) {
		struct bucket_state *bs = &state.buckets[i];

		if (i != 0) {
			bs->alloc_limit = ALIGN_SIZE(BUCKET_NEXT(state.buckets[i-1].alloc_limit));
		}

		/* work out how many elements can be put on each page */
		bs->elements_per_page = state.page_size/bs->alloc_limit;
		while (bs->elements_per_page*bs->alloc_limit + PAGE_HEADER_SIZE(bs->elements_per_page) >
		       state.page_size) {
			bs->elements_per_page--;
		}
		if (unlikely(bs->elements_per_page <= 1)) {
			break;
		}
	}
	state.max_bucket = i-1;
	state.largest_bucket = state.buckets[state.max_bucket].alloc_limit;
	state.initialised = true;
}


/*
  large allocation function - use mmap per allocation
 */
static void *alloc_large(size_t size)
{
	uint32_t num_pages;
	struct large_header *lh;

	num_pages = (size+ALIGN_COST(struct large_header)+(state.page_size-1))/state.page_size;

	/* check for wrap */
	if (unlikely(num_pages < size/state.page_size)) {
		return NULL;
	}

	lh = (struct large_header *)mmap(0, num_pages*state.page_size, PROT_READ | PROT_WRITE, 
					 MAP_ANON | MAP_PRIVATE, -1, 0);
	if (unlikely(lh == (struct large_header *)-1)) {
		return NULL;
	}

	lh->num_pages = num_pages;

	return (void *)ALIGN_UP(lh, struct large_header);
}


/*
  large free function
 */
static void alloc_large_free(void *ptr)
{
	struct large_header *lh = ALIGN_DOWN_PAGE(ptr, struct large_header);
	munmap((void *)lh, lh->num_pages*state.page_size);
}

/*
  refill a bucket
 */
static void alloc_refill_bucket(struct bucket_state *bs)
{
	struct page_header *ph;

	/* see if we can get a page from the global empty list */
	ph = state.empty_list;
	if (ph != NULL) {
		MOVE_LIST_FRONT(state.empty_list, bs->page_list, ph);
		ph->num_elements = bs->elements_per_page;
		ph->bucket = bs;
		memset(ph->used, 0, (ph->num_elements+7)/8);
		state.num_empty_pages--;
		return;
	}

	/* we need to allocate a new page */
	ph = (struct page_header *)mmap(0, state.page_size, PROT_READ | PROT_WRITE, 
					MAP_ANON | MAP_PRIVATE, -1, 0);
	if (unlikely(ph == (struct page_header *)-1)) {
		return;
	}

	/* we rely on mmap() giving us initially zero memory */
	ph->bucket = bs;
	ph->num_elements = bs->elements_per_page;

	/* link it into the page_list */
	ph->next = bs->page_list;
	if (ph->next) ph->next->prev = ph;
	bs->page_list = ph;
}


/*
  find the first free element in a bitmap. This assumes there is a bit
  to be found! Any return >= num_bits means an error (no free bit found)
 */
static inline unsigned alloc_find_free_bit(const uint32_t *bitmap, uint32_t num_bits)
{
	uint32_t i;
	const uint32_t n = (num_bits+31)/32;
	for (i=0;i<n;i++) {
		if (bitmap[i] != 0xFFFFFFFF) {
			return (i*32) + ffs(~bitmap[i]) - 1;
		}
	}
	return num_bits;
}


/*
  allocate some memory
 */
static void *alloc_malloc(size_t size)
{
	uint32_t i;
	struct bucket_state *bs;
	struct page_header *ph;

	if (unlikely(state.initialised == false)) {
		alloc_initialise();
	}

	/* is it a large allocation? */
	if (unlikely(size > state.largest_bucket)) {
		return alloc_large(size);
	}

	/* find the right bucket */
	for (i=0;i<=state.max_bucket;i++) {
		if (state.buckets[i].alloc_limit >= size) {
			break;
		}
	}

	bs = &state.buckets[i];

	/* it might be empty. If so, refill it */
	if (unlikely(bs->page_list == NULL)) {
		alloc_refill_bucket(bs);
		if (unlikely(bs->page_list == NULL)) {
			return NULL;
		}
	}

	/* take the first one */
	ph = bs->page_list;
	i = alloc_find_free_bit(ph->used, ph->num_elements);
	if (unlikely(i >= ph->num_elements)) {
		abort();
	}

	ph->elements_used++;

	/* mark this element as used */
	ph->used[i/32] |= 1<<(i%32);

	/* check if this page is now full */
	if (unlikely(ph->elements_used == ph->num_elements)) {
		MOVE_LIST_FRONT(bs->page_list, bs->full_list, ph);
	}

	/* return the element within the page */
	return (void *)((i*bs->alloc_limit)+PAGE_HEADER_SIZE(ph->num_elements)+(intptr_t)ph);
}


/*
  release all completely free pages
 */
static void alloc_release(void)
{
	struct page_header *ph, *next;

	/* release all the empty pages */
	for (ph=state.empty_list;ph;ph=next) {
		next = ph->next;
		munmap((void *)ph, state.page_size);
	}
	state.empty_list = NULL;
	state.num_empty_pages = 0;
}


/*
  free some memory
 */
static void alloc_free(void *ptr)
{
	struct page_header *ph = ALIGN_DOWN_PAGE(ptr, struct page_header);
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

	/* mark the block as being free in the page bitmap */
	i = ((((intptr_t)ptr) - (intptr_t)ph) - PAGE_HEADER_SIZE(ph->num_elements)) / 
		bs->alloc_limit;
	ph->used[i/32] &= ~(1<<(i%32));

	/* if all blocks in this page were previously used, then move
	   the page from the full list to the page list */
	if (unlikely(ph->elements_used == ph->num_elements)) {
		MOVE_LIST(bs->full_list, bs->page_list, ph);
	}

	ph->elements_used--;

	/* if all blocks in the page are now free, then its an empty
	   page, and can be added to the global pool of empty
	   pages. When that gets too large, release them completely */
	if (unlikely(ph->elements_used == 0)) {
		MOVE_LIST(bs->page_list, state.empty_list, ph);
		state.num_empty_pages++;
		if (state.num_empty_pages == FREE_PAGE_LIMIT) {
			alloc_release();
		}
	}
}


/*
  realloc for large blocks
 */
static void *alloc_realloc_large(void *ptr, size_t size)
{
	void *ptr2;
	struct large_header *lh = ALIGN_DOWN_PAGE(ptr, struct large_header);

	/* check for wrap around */
	if (size + ALIGN_COST(struct large_header) < size) {
		return NULL;
	}

	/* if it fits in the current pointer, keep it there */
	if (lh->num_pages*state.page_size >= (size + ALIGN_COST(struct large_header))) {
		return ptr;
	}

	ptr2 = alloc_malloc(size);
	if (ptr2 == NULL) {
		return NULL;
	}

	memcpy(ptr2, ptr, 
	       MIN(size, (lh->num_pages*state.page_size)-ALIGN_COST(struct large_header)));
	alloc_large_free(ptr);
	return ptr2;
}



/*
  realloc
 */
static void *alloc_realloc(void *ptr, size_t size)
{
	struct page_header *ph = ALIGN_DOWN_PAGE(ptr, struct page_header);
	struct bucket_state *bs;
	void *ptr2;

	/* modern realloc() semantics */
	if (unlikely(size == 0)) {
		alloc_free(ptr);
		return NULL;
	}
	if (unlikely(ptr == NULL)) {
		return alloc_malloc(size);
	}

	/* if it was a large allocation, it needs special handling */
	if (ph->num_pages != 0) {
		return alloc_realloc_large(ptr, size);
	}	

	bs = ph->bucket;
	if (bs->alloc_limit >= size) {
		return ptr;
	}

	ptr2 = alloc_malloc(size);
	if (unlikely(ptr2 == NULL)) {
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
	uint32_t total_size = nmemb*size;
	if (unlikely(size != 0 && total_size / size != nmemb)) {
		return NULL;
	}
	if (unlikely(total_size > state.largest_bucket)) {
		if (unlikely(state.initialised == false)) {
			alloc_initialise();
		}
		return alloc_large(total_size);
	}
	ptr = alloc_malloc(total_size);
	if (likely(ptr != NULL)) {
		memset(ptr, 0, total_size);
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
