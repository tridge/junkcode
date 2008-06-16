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

     - fast for small allocations
     - all memory comes from anonymous mmap
     - releases memory back to the OS as much as possible
     - no per-block prefix (so extremely low overhead)
     - very little paranoid checking in the code
     - thread safety optional at compile time 

  Design:

      allocations are either 'small' or 'large' or 'memalign'

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

      'memalign' allocations are for handling memlign()
      requests. These are horribly inefficient, using a singly linked
      list. Best to avoid this function.
      
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/mman.h>
#ifdef THREAD_SAFE
#include <pthread.h>
#endif

#if (__GNUC__ >= 3)
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

#define MAX_BUCKETS     15
#define BASE_BUCKET     16
#define FREE_PAGE_LIMIT  2
#define ALLOC_ALIGNMENT 16
#define BUCKET_NEXT_SIZE(prev) ((3*(prev))/2)
#define PID_CHECK        0
#define BUCKET_LOOKUP_SIZE 128
#define ALLOC_PARANOIA   0

/* you can hard code the page size in the Makefile for more speed */
#ifndef PAGE_SIZE
#define PAGE_SIZE state.page_size
#endif

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
#define ALIGN_DOWN_PAGE(ptr, type) (type *)((~(intptr_t)(PAGE_SIZE-1)) & (intptr_t)(ptr))

/* work out the aligned size of a page header, given the size of the used[] array */
#define PAGE_HEADER_SIZE(n) ALIGN_SIZE(offsetof(struct page_header, used) + 4*(((n)+31)/32))

/* see if a pointer is page aligned. This detects large memalign() pointers */
#define IS_PAGE_ALIGNED(ptr) (((PAGE_SIZE-1) & (intptr_t)ptr) == 0)

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
#if PID_CHECK
	pid_t pid;
#endif
	struct page_header *next, *prev;
	struct bucket_state *bucket;
	uint32_t used[1];
};

struct large_header {
	uint32_t num_pages;
};

struct memalign_header {
	struct memalign_header *next;
	void *p;
	uint32_t num_pages;
};

struct bucket_state {
	uint32_t alloc_limit;
	uint16_t elements_per_page;
#ifdef THREAD_SAFE
	pthread_mutex_t mutex;
#endif
	struct page_header *page_list;
	struct page_header *full_list;
};

struct alloc_state {
	bool initialised;
	uint32_t page_size;
	uint32_t largest_bucket;
	uint32_t max_bucket;
#if PID_CHECK
	pid_t pid;
#endif
	/* this is a lookup table that maps size/ALLOC_ALIGNMENT to the right bucket */
	uint8_t bucket_lookup[BUCKET_LOOKUP_SIZE];

	/* the rest of the structure may be written to after initialisation,
	   so all accesses need to hold the mutex */
#ifdef THREAD_SAFE
	pthread_mutex_t mutex;
#endif
	uint32_t num_empty_pages;
	struct page_header *empty_list;
	struct memalign_header *memalign_list;
	struct bucket_state buckets[MAX_BUCKETS];

};

/* static state of the allocator */
#ifndef THREAD_SAFE
static struct alloc_state state;
#else
static struct alloc_state state = {
	.mutex = PTHREAD_MUTEX_INITIALIZER
};
#endif

#ifdef THREAD_SAFE
#define THREAD_LOCK(mutexp) do { if (pthread_mutex_lock(mutexp) != 0) abort(); } while (0);
#define THREAD_UNLOCK(mutexp) do { if (pthread_mutex_unlock(mutexp) != 0) abort(); } while (0);
#else
#define THREAD_LOCK(mutexp) 
#define THREAD_UNLOCK(mutexp) 
#endif

/*
  initialise the allocator
 */
static void alloc_initialise(void)
{
	int i, b=0, n;
	THREAD_LOCK(&state.mutex);
	state.page_size = getpagesize();
	if (PAGE_SIZE != state.page_size) {
		abort();
	}
	for (i=0;i<MAX_BUCKETS;i++) {
		struct bucket_state *bs = &state.buckets[i];

#ifdef THREAD_SAFE
		pthread_mutex_init(&bs->mutex, NULL);
#endif

		if (i == 0) {
			int n;
			bs->alloc_limit = BASE_BUCKET;
			n = bs->alloc_limit/ALLOC_ALIGNMENT;
			if (n > BUCKET_LOOKUP_SIZE) {
				n = BUCKET_LOOKUP_SIZE;
			}
			memset(&state.bucket_lookup[0], i, n);
			b = n;
		} else {
			struct bucket_state *bs0 = &state.buckets[i-1];
			bs->alloc_limit = ALIGN_SIZE(BUCKET_NEXT_SIZE(bs0->alloc_limit));

			/* squeeze in one more bucket if possible */
			if (bs->alloc_limit > PAGE_SIZE - PAGE_HEADER_SIZE(1)) {
				bs->alloc_limit = PAGE_SIZE - PAGE_HEADER_SIZE(1);
				if (bs->alloc_limit == bs0->alloc_limit) break;
			}

			n = (bs->alloc_limit - bs0->alloc_limit)/ALLOC_ALIGNMENT;
			if (n + b > BUCKET_LOOKUP_SIZE) {
				n = BUCKET_LOOKUP_SIZE - b;
			}
			memset(&state.bucket_lookup[b], i, n);
			b += n;
		}

		/* work out how many elements can be put on each page */
		bs->elements_per_page = (PAGE_SIZE-PAGE_HEADER_SIZE(1))/bs->alloc_limit;
		while (bs->elements_per_page*bs->alloc_limit + PAGE_HEADER_SIZE(bs->elements_per_page) >
		       PAGE_SIZE) {
			bs->elements_per_page--;
		}
		if (unlikely(bs->elements_per_page < 1)) {
			break;
		}
	}
	state.max_bucket = i-1;
	state.largest_bucket = state.buckets[state.max_bucket].alloc_limit;
	state.initialised = true;
#if PID_CHECK
	state.pid = getpid();
#endif
	THREAD_UNLOCK(&state.mutex);
}

#if PID_CHECK
static void alloc_pid_handler(void)
{
	state.pid = getpid();
}
#endif

/*
  large allocation function - use mmap per allocation
 */
static void *alloc_large(size_t size)
{
	uint32_t num_pages;
	struct large_header *lh;

	num_pages = (size+ALIGN_COST(struct large_header)+(PAGE_SIZE-1))/PAGE_SIZE;

	/* check for wrap */
	if (unlikely(num_pages < size/PAGE_SIZE)) {
		return NULL;
	}

	lh = (struct large_header *)mmap(0, num_pages*PAGE_SIZE, PROT_READ | PROT_WRITE, 
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
	munmap((void *)lh, lh->num_pages*PAGE_SIZE);
}

/*
  refill a bucket
  called with the bucket lock held
 */
static void alloc_refill_bucket(struct bucket_state *bs)
{
	struct page_header *ph;

	/* see if we can get a page from the global empty list */
	THREAD_LOCK(&state.mutex);
	ph = state.empty_list;
	if (ph != NULL) {
		MOVE_LIST_FRONT(state.empty_list, bs->page_list, ph);
		ph->num_elements = bs->elements_per_page;
		ph->bucket = bs;
#if PID_CHECK
		ph->pid = getpid();
#endif
		memset(ph->used, 0, (ph->num_elements+7)/8);
		state.num_empty_pages--;
		THREAD_UNLOCK(&state.mutex);
		return;
	}

	/* we need to allocate a new page */
	ph = (struct page_header *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, 
					MAP_ANON | MAP_PRIVATE, -1, 0);
	if (unlikely(ph == (struct page_header *)-1)) {
		THREAD_UNLOCK(&state.mutex);
		return;
	}

	/* we rely on mmap() giving us initially zero memory */
	ph->bucket = bs;
	ph->num_elements = bs->elements_per_page;
#if PID_CHECK
	ph->pid = getpid();
#endif

	/* link it into the page_list */
	ph->next = bs->page_list;
	if (ph->next) ph->next->prev = ph;
	bs->page_list = ph;
	THREAD_UNLOCK(&state.mutex);
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

#if PID_CHECK
	if (unlikely(state.pid != getpid())) {
		alloc_pid_handler();
	}
#endif

	/* is it a large allocation? */
	if (unlikely(size > state.largest_bucket)) {
		return alloc_large(size);
	}

	/* find the right bucket */
	if (likely(size / ALLOC_ALIGNMENT < BUCKET_LOOKUP_SIZE)) {
		if (size == 0) {
			i = 0;
		} else {
			i = state.bucket_lookup[(size-1) / ALLOC_ALIGNMENT];
		}
	} else {
		for (i=state.bucket_lookup[BUCKET_LOOKUP_SIZE-1];i<=state.max_bucket;i++) {
			if (state.buckets[i].alloc_limit >= size) {
				break;
			}
		}
	}

	bs = &state.buckets[i];

	THREAD_LOCK(&bs->mutex);

#if ALLOC_PARANOIA
	if (size > bs->alloc_limit) {
		abort();
	}
#endif

	/* it might be empty. If so, refill it */
	if (unlikely(bs->page_list == NULL)) {
		alloc_refill_bucket(bs);
		if (unlikely(bs->page_list == NULL)) {
			THREAD_UNLOCK(&bs->mutex);
			return NULL;
		}
	}

#if PID_CHECK
	if (unlikely(bs->page_list->pid != getpid() && 
		     bs->page_list->elements_used >= bs->page_list->num_elements/2)) {
		alloc_refill_bucket(bs);
	}
#endif

	/* take the first one */
	ph = bs->page_list;
	i = alloc_find_free_bit(ph->used, ph->num_elements);
#if ALLOC_PARANOIA
	if (unlikely(i >= ph->num_elements)) {
		abort();
	}
#endif
	ph->elements_used++;

	/* mark this element as used */
	ph->used[i/32] |= 1<<(i%32);

	/* check if this page is now full */
	if (unlikely(ph->elements_used == ph->num_elements)) {
		MOVE_LIST_FRONT(bs->page_list, bs->full_list, ph);
	}

	THREAD_UNLOCK(&bs->mutex);

	/* return the element within the page */
	return (void *)((i*bs->alloc_limit)+PAGE_HEADER_SIZE(ph->num_elements)+(intptr_t)ph);
}


/*
  release all completely free pages
  called with state.mutex held
 */
static void alloc_release(void)
{
	struct page_header *ph, *next;

	/* release all the empty pages */
	for (ph=state.empty_list;ph;ph=next) {
		next = ph->next;
		munmap((void *)ph, PAGE_SIZE);
	}
	state.empty_list = NULL;
	state.num_empty_pages = 0;
}

/*
  free some memory from a alloc_memalign
 */
static void alloc_memalign_free(void *ptr)
{
	struct memalign_header *mh;

	THREAD_LOCK(&state.mutex);

#if ALLOC_PARANOIA	
	if (state.memalign_list == NULL) {
		abort();
	}
#endif

	/* see if its the top one */
	mh = state.memalign_list;
	if (mh->p == ptr) {
		state.memalign_list = mh->next;
		munmap(ptr, mh->num_pages*PAGE_SIZE);
		THREAD_UNLOCK(&state.mutex);
		free(mh);
		return;
	}

	/* otherwise walk it */
	while (mh->next) {
		struct memalign_header *mh2 = mh->next;
		if (mh2->p == ptr) {
			mh->next = mh2->next;
			munmap(ptr, mh2->num_pages);
			THREAD_UNLOCK(&state.mutex);
			free(mh2);
			return;
		}
		mh = mh->next;
	}

	THREAD_UNLOCK(&state.mutex);

#if ALLOC_PARANOIA
	/* it wasn't there ... */
	abort();
#endif
}


/*
  realloc for memalign blocks
 */
static void *alloc_memalign_realloc(void *ptr, size_t size)
{
	struct memalign_header *mh;
	void *ptr2;

	THREAD_LOCK(&state.mutex);
	for (mh=state.memalign_list;mh;mh=mh->next) {
		if (mh->p == ptr) break;
	}
#if ALLOC_PARANOIA
	if (mh == NULL) {
		abort();
	}
#endif
	THREAD_UNLOCK(&state.mutex);

	ptr2 = malloc(size);
	if (ptr2 == NULL) {
		return NULL;
	}

	memcpy(ptr2, ptr, MIN(size, mh->num_pages*PAGE_SIZE));
	free(ptr);
	return ptr2;
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

	if (unlikely(IS_PAGE_ALIGNED(ptr))) {
		alloc_memalign_free(ptr);
		return;
	}

	if (ph->num_pages != 0) {
		alloc_large_free(ptr);
		return;
	}

	bs = ph->bucket;
	THREAD_LOCK(&bs->mutex);

	/* mark the block as being free in the page bitmap */
	i = ((((intptr_t)ptr) - (intptr_t)ph) - PAGE_HEADER_SIZE(ph->num_elements)) / 
		bs->alloc_limit;
#if ALLOC_PARANOIA
	if ((ph->used[i/32] & (1<<(i%32))) == 0) {
		abort();
	}
#endif
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
		THREAD_LOCK(&state.mutex);
		MOVE_LIST(bs->page_list, state.empty_list, ph);
		state.num_empty_pages++;
		if (state.num_empty_pages == FREE_PAGE_LIMIT) {
			alloc_release();
		}
		THREAD_UNLOCK(&state.mutex);
	}
	THREAD_UNLOCK(&bs->mutex);
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
	if (lh->num_pages*PAGE_SIZE >= (size + ALIGN_COST(struct large_header))) {
		return ptr;
	}

	ptr2 = alloc_malloc(size);
	if (ptr2 == NULL) {
		return NULL;
	}

	memcpy(ptr2, ptr, 
	       MIN(size, (lh->num_pages*PAGE_SIZE)-ALIGN_COST(struct large_header)));
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

	if (unlikely(IS_PAGE_ALIGNED(ptr))) {
		return alloc_memalign_realloc(ptr, size);
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
#if PID_CHECK
	if (unlikely(state.pid != getpid())) {
		alloc_pid_handler();
	}
#endif
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


/*
  memalign - the most difficult of the lot with these data structures 

  this is O(n) in the number of aligned allocations. Don't use
  memalign() unless you _really_ need it
 */
static void *alloc_memalign(size_t boundary, size_t size)
{
	struct memalign_header *mh;
	uint32_t extra_pages=0;

#if PID_CHECK
	if (unlikely(state.pid != getpid())) {
		alloc_pid_handler();
	}
#endif

	if (unlikely(state.initialised == false)) {
		alloc_initialise();
	}

	/* must be power of 2, and no overflow on page alignment */
	if (boundary & (boundary-1)) {
		return NULL;
	}
	if (boundary == 0 || (size+PAGE_SIZE-1 < size)) {
		return NULL;
	}

	/* try and get lucky */
	if (boundary < state.largest_bucket) {
		void *ptr = malloc(size);
		if (((intptr_t)ptr) % boundary == 0) {
			return ptr;
		}

		/* nope, do it the slow way */
		free(ptr);
	}

	mh = malloc(sizeof(struct memalign_header));
	if (mh == NULL) {
		return NULL;
	}

	/* need to overallocate to guarantee alignment of larger than
	   a page */
	if (boundary > PAGE_SIZE) {
		extra_pages = boundary / PAGE_SIZE;
	}

	/* give them a page aligned allocation */
	mh->num_pages = (size + PAGE_SIZE-1) / PAGE_SIZE;

	if (mh->num_pages == 0) {
		mh->num_pages = 1;
	}

	/* check for overflow due to large boundary */
	if (mh->num_pages + extra_pages < mh->num_pages) {
		free(mh);
		return NULL;
	}

	mh->p = mmap(0, (extra_pages+mh->num_pages)*PAGE_SIZE, PROT_READ | PROT_WRITE, 
		     MAP_ANON | MAP_PRIVATE, -1, 0);
	if (mh->p == (void *)-1) {
		free(mh);
		return NULL;
	}
	if (extra_pages) {
		while (((intptr_t)mh->p) % boundary) {
			munmap(mh->p, PAGE_SIZE);
			mh->p = (void *)(PAGE_SIZE + (intptr_t)mh->p);
			extra_pages--;
		}
		if (extra_pages) {
			munmap((void *)(mh->num_pages*PAGE_SIZE + (intptr_t)mh->p), 
			       extra_pages*PAGE_SIZE);
		}
	}

	THREAD_LOCK(&state.mutex);
	mh->next = state.memalign_list;
	state.memalign_list = mh;
	THREAD_UNLOCK(&state.mutex);

	return mh->p;
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

void cfree(void *ptr)
{
	alloc_free(ptr);
}

void *realloc(void *ptr, size_t size)
{
	return alloc_realloc(ptr, size);
}

void *memalign(size_t boundary, size_t size)
{
	return alloc_memalign(boundary, size);
}

void *valloc(size_t size)
{
	return alloc_memalign(getpagesize(), size);
}

void *pvalloc(size_t size)
{
	return alloc_memalign(getpagesize(), size);
}

int posix_memalign(void **memptr, size_t alignment, size_t size)
{
	*memptr = alloc_memalign(alignment, size);
	if (*memptr == NULL) {
		errno = ENOMEM;
		return -1;
	}
	return 0;
}

/* try to catch the internal entry points too */
#if (__GNUC__ >= 3)
void *__libc_malloc(size_t size)                                     __attribute__((weak, alias("malloc")));
void *__libc_calloc(size_t nmemb, size_t size)                       __attribute__((weak, alias("calloc")));
void __libc_free(void *ptr)                                          __attribute__((weak, alias("free")));
void __libc_cfree(void *ptr)                                         __attribute__((weak, alias("cfree")));
void *__libc_realloc(void *ptr, size_t size)                         __attribute__((weak, alias("realloc")));
void *__libc_memalign(size_t boundary, size_t size)                  __attribute__((weak, alias("memalign")));
void *__libc_valloc(size_t size)                                     __attribute__((weak, alias("valloc")));
void *__libc_pvalloc(size_t size)                                    __attribute__((weak, alias("pvalloc")));
int __posix_memalign(void **memptr, size_t alignment, size_t size)   __attribute__((weak, alias("posix_memalign")));
#endif

#endif
