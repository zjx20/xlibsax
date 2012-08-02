#include <string.h>
#include <assert.h>

#include "mempool.h"
#include "os_api.h"

/** 
 * @file mempool.c
 * -# SLAB memory allocator, ported from nginx
 * -# FSB memory allocator, ported from SEDA
 */

#define sys_align_ptr(p, a) \
    (uint8_t *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


typedef struct slab_page_s  slab_page_t;

struct slab_page_s {
    uintptr_t     slab;
    slab_page_t  *next;
    uintptr_t     prev;
};


struct slab_pool_t {
    size_t       min_size;
    size_t       min_shift;

    slab_page_t  *pages;
    slab_page_t  free;

    uint8_t      *start;
    uint8_t      *end;
};

#define SLAB_PAGE_MASK   3
#define SLAB_PAGE        0
#define SLAB_BIG         1
#define SLAB_EXACT       2
#define SLAB_SMALL       3


#if defined(__x86_64__) || defined(_WIN64) /* (PTR_SIZE == 8) */

#define SLAB_PAGE_FREE   0
#define SLAB_PAGE_BUSY   0xffffffffffffffff
#define SLAB_PAGE_START  0x8000000000000000

#define SLAB_SHIFT_MASK  0x000000000000000f
#define SLAB_MAP_MASK    0xffffffff00000000
#define SLAB_MAP_SHIFT   32

#define SLAB_BUSY        0xffffffffffffffff

#else /* (PTR_SIZE == 4) */

#define SLAB_PAGE_FREE   0
#define SLAB_PAGE_BUSY   0xffffffff
#define SLAB_PAGE_START  0x80000000

#define SLAB_SHIFT_MASK  0x0000000f
#define SLAB_MAP_MASK    0xffff0000
#define SLAB_MAP_SHIFT   16

#define SLAB_BUSY        0xffffffff

#endif /* (PTR_SIZE == ?) */

static uintptr_t  sys_pagesize = 0;
static uintptr_t  sys_pagesize_shift = 0;

static uintptr_t  slab_max_size = 0;
static uintptr_t  slab_exact_size = 0;
static uintptr_t  slab_exact_shift = 0;

void g_slab_setup(size_t page_size)
{
	uintptr_t  n;
	if (sys_pagesize) return;
	
    sys_pagesize = page_size;
    for (n = sys_pagesize; n >>= 1; sys_pagesize_shift++) ;

    slab_max_size = sys_pagesize / 2;
    slab_exact_size = sys_pagesize / (8 * sizeof(uintptr_t));
    for (n = slab_exact_size; n >>= 1; slab_exact_shift++) ;
}

struct slab_pool_t *g_slab_init(void *addr, size_t total)
{
    uint8_t      *p;
    size_t       size;
    intptr_t     m;
    uintptr_t    i, n, pages;
    slab_page_t  *slots;
    
    struct slab_pool_t *pool = (struct slab_pool_t *)addr;
    pool->min_shift = 3;
    pool->end = (uint8_t *) addr + total;

    if (slab_max_size == 0) return 0; /* not setup yet! */

    pool->min_size = 1 << pool->min_shift;

    p = (uint8_t *) pool + sizeof(struct slab_pool_t);
    size = pool->end - p;

    slots = (slab_page_t *) p;
    n = sys_pagesize_shift - pool->min_shift;

    for (i = 0; i < n; i++) {
        slots[i].slab = 0;
        slots[i].next = &slots[i];
        slots[i].prev = 0;
    }

    p += n * sizeof(slab_page_t);

    pages = (uintptr_t) (size / (sys_pagesize + sizeof(slab_page_t)));

    (void)memset(p, 0, pages * sizeof(slab_page_t));

    pool->pages = (slab_page_t *) p;

    pool->free.prev = 0;
    pool->free.next = (slab_page_t *) p;

    pool->pages->slab = pages;
    pool->pages->next = &pool->free;
    pool->pages->prev = (uintptr_t) &pool->free;

    pool->start = (uint8_t *)
                  sys_align_ptr((uintptr_t) p + pages * sizeof(slab_page_t),
                                 sys_pagesize);

    m = pages - (pool->end - pool->start) / sys_pagesize;
    if (m > 0) {
        pages -= m;
        pool->pages->slab = pages;
    }
    return pool;
}

static slab_page_t *
slab_alloc_pages(struct slab_pool_t *pool, uintptr_t pages)
{
    slab_page_t  *page, *p;

    for (page = pool->free.next; page != &pool->free; page = page->next) {

        if (page->slab >= pages) {

            if (page->slab > pages) {
                page[pages].slab = page->slab - pages;
                page[pages].next = page->next;
                page[pages].prev = page->prev;

                p = (slab_page_t *) page->prev;
                p->next = &page[pages];
                page->next->prev = (uintptr_t) &page[pages];

            } else {
                p = (slab_page_t *) page->prev;
                p->next = page->next;
                page->next->prev = page->prev;
            }

            page->slab = pages | SLAB_PAGE_START;
            page->next = NULL;
            page->prev = SLAB_PAGE;

            if (--pages == 0) {
                return page;
            }

            for (p = page + 1; pages; pages--) {
                p->slab = SLAB_PAGE_BUSY;
                p->next = NULL;
                p->prev = SLAB_PAGE;
                p++;
            }

            return page;
        }
    }

    return NULL;
}


static void
slab_free_pages(struct slab_pool_t *pool, slab_page_t *page,
    uintptr_t pages)
{
    slab_page_t  *prev;

    page->slab = pages--;

    if (pages) {
        (void)memset(&page[1], 0, pages * sizeof(slab_page_t));
    }

    if (page->next) {
        prev = (slab_page_t *) (page->prev & ~SLAB_PAGE_MASK);
        prev->next = page->next;
        page->next->prev = page->prev;
    }

    page->prev = (uintptr_t) &pool->free;
    page->next = pool->free.next;

    page->next->prev = (uintptr_t) page;

    pool->free.next = page;
}

void *g_slab_alloc(struct slab_pool_t *pool, size_t size)
{
   size_t            s;
    uintptr_t         p, n, m, mask, *bitmap;
    uintptr_t        i, slot, shift, map;
    slab_page_t  *page, *prev, *slots;

    if (size >= slab_max_size) {

        page = slab_alloc_pages(pool, (size + sys_pagesize - 1)
                                          >> sys_pagesize_shift);
        if (page) {
            p = (page - pool->pages) << sys_pagesize_shift;
            p += (uintptr_t) pool->start;

        } else {
            p = 0;
        }

        goto done;
    }

    if (size > pool->min_size) {
        shift = 1;
        for (s = size - 1; s >>= 1; shift++) { /* void */ }
        slot = shift - pool->min_shift;

    } else {
        size = pool->min_size;
        shift = pool->min_shift;
        slot = 0;
    }

    slots = (slab_page_t *) ((uint8_t *) pool + sizeof(struct slab_pool_t));
    page = slots[slot].next;

    if (page->next != page) {

        if (shift < slab_exact_shift) {

            do {
                p = (page - pool->pages) << sys_pagesize_shift;
                bitmap = (uintptr_t *) (pool->start + p);

                map = (1 << (sys_pagesize_shift - shift))
                          / (sizeof(uintptr_t) * 8);

                for (n = 0; n < map; n++) {

                    if (bitmap[n] != SLAB_BUSY) {

                        for (m = 1, i = 0; m; m <<= 1, i++) {
                            if ((bitmap[n] & m)) {
                                continue;
                            }

                            bitmap[n] |= m;

                            i = ((n * sizeof(uintptr_t) * 8) << shift)
                                + (i << shift);

                            if (bitmap[n] == SLAB_BUSY) {
                                for (n = n + 1; n < map; n++) {
                                     if (bitmap[n] != SLAB_BUSY) {
                                         p = (uintptr_t) bitmap + i;

                                         goto done;
                                     }
                                }

                                prev = (slab_page_t *)
                                            (page->prev & ~SLAB_PAGE_MASK);
                                prev->next = page->next;
                                page->next->prev = page->prev;

                                page->next = NULL;
                                page->prev = SLAB_SMALL;
                            }

                            p = (uintptr_t) bitmap + i;

                            goto done;
                        }
                    }
                }

                page = page->next;

            } while (page);

        } else if (shift == slab_exact_shift) {

            do {
                if (page->slab != SLAB_BUSY) {

                    for (m = 1, i = 0; m; m <<= 1, i++) {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;

                        if (page->slab == SLAB_BUSY) {
                            prev = (slab_page_t *)
                                            (page->prev & ~SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = SLAB_EXACT;
                        }

                        p = (page - pool->pages) << sys_pagesize_shift;
                        p += i << shift;
                        p += (uintptr_t) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);

        } else { /* shift > slab_exact_shift */

            n = sys_pagesize_shift - (page->slab & SLAB_SHIFT_MASK);
            n = 1 << n;
            n = ((uintptr_t) 1 << n) - 1;
            mask = n << SLAB_MAP_SHIFT;

            do {
                if ((page->slab & SLAB_MAP_MASK) != mask) {

                    for (m = (uintptr_t) 1 << SLAB_MAP_SHIFT, i = 0;
                         m & mask;
                         m <<= 1, i++)
                    {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;

                        if ((page->slab & SLAB_MAP_MASK) == mask) {
                            prev = (slab_page_t *)
                                            (page->prev & ~SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = SLAB_BIG;
                        }

                        p = (page - pool->pages) << sys_pagesize_shift;
                        p += i << shift;
                        p += (uintptr_t) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);
        }
    }

    page = slab_alloc_pages(pool, 1);

    if (page) {
        if (shift < slab_exact_shift) {
            p = (page - pool->pages) << sys_pagesize_shift;
            bitmap = (uintptr_t *) (pool->start + p);

            s = 1 << shift;
            n = (1 << (sys_pagesize_shift - shift)) / 8 / s;

            if (n == 0) {
                n = 1;
            }

            bitmap[0] = (2 << n) - 1;

            map = (1 << (sys_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (i = 1; i < map; i++) {
                bitmap[i] = 0;
            }

            page->slab = shift;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | SLAB_SMALL;

            slots[slot].next = page;

            p = ((page - pool->pages) << sys_pagesize_shift) + s * n;
            p += (uintptr_t) pool->start;

            goto done;

        } else if (shift == slab_exact_shift) {

            page->slab = 1;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | SLAB_EXACT;

            slots[slot].next = page;

            p = (page - pool->pages) << sys_pagesize_shift;
            p += (uintptr_t) pool->start;

            goto done;

        } else { /* shift > slab_exact_shift */

            page->slab = ((uintptr_t) 1 << SLAB_MAP_SHIFT) | shift;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | SLAB_BIG;

            slots[slot].next = page;

            p = (page - pool->pages) << sys_pagesize_shift;
            p += (uintptr_t) pool->start;

            goto done;
        }
    }

    p = 0;

done:

    return (void *) p;
}


void g_slab_free(struct slab_pool_t *pool, void *p)
{
    size_t            size;
    uintptr_t         slab, m, *bitmap;
    uintptr_t        n, type, slot, shift, map;
    slab_page_t  *slots, *page;


    if ((uint8_t *) p < pool->start || (uint8_t *) p > pool->end) {
        goto fail;
    }

    n = ((uint8_t *) p - pool->start) >> sys_pagesize_shift;
    page = &pool->pages[n];
    slab = page->slab;
    type = page->prev & SLAB_PAGE_MASK;

    switch (type) {

    case SLAB_SMALL:

        shift = slab & SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        n = ((uintptr_t) p & (sys_pagesize - 1)) >> shift;
        m = (uintptr_t) 1 << (n & (sizeof(uintptr_t) * 8 - 1));
        n /= (sizeof(uintptr_t) * 8);
        bitmap = (uintptr_t *) ((uintptr_t) p & ~(sys_pagesize - 1));

        if (bitmap[n] & m) {

            if (page->next == NULL) {
                slots = (slab_page_t *)
                                   ((uint8_t *) pool + sizeof(struct slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | SLAB_SMALL;
                page->next->prev = (uintptr_t) page | SLAB_SMALL;
            }

            bitmap[n] &= ~m;

            n = (1 << (sys_pagesize_shift - shift)) / 8 / (1 << shift);

            if (n == 0) {
                n = 1;
            }

            if (bitmap[0] & ~(((uintptr_t) 1 << n) - 1)) {
                goto done;
            }

            map = (1 << (sys_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (n = 1; n < map; n++) {
                if (bitmap[n]) {
                    goto done;
                }
            }

            slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case SLAB_EXACT:

        m = (uintptr_t) 1 <<
                (((uintptr_t) p & (sys_pagesize - 1)) >> slab_exact_shift);
        size = slab_exact_size;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        if (slab & m) {
            if (slab == SLAB_BUSY) {
                slots = (slab_page_t *)
                                   ((uint8_t *) pool + sizeof(struct slab_pool_t));
                slot = slab_exact_shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | SLAB_EXACT;
                page->next->prev = (uintptr_t) page | SLAB_EXACT;
            }

            page->slab &= ~m;

            if (page->slab) {
                goto done;
            }

            slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case SLAB_BIG:

        shift = slab & SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        m = (uintptr_t) 1 << ((((uintptr_t) p & (sys_pagesize - 1)) >> shift)
                              + SLAB_MAP_SHIFT);

        if (slab & m) {

            if (page->next == NULL) {
                slots = (slab_page_t *)
                                   ((uint8_t *) pool + sizeof(struct slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | SLAB_BIG;
                page->next->prev = (uintptr_t) page | SLAB_BIG;
            }

            page->slab &= ~m;

            if (page->slab & SLAB_MAP_MASK) {
                goto done;
            }

            slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case SLAB_PAGE:

        if ((uintptr_t) p & (sys_pagesize - 1)) {
            goto wrong_chunk;
        }

        if (slab == SLAB_PAGE_FREE) {
            goto fail;
        }

        if (slab == SLAB_PAGE_BUSY) {
            goto fail;
        }

        n = ((uint8_t *) p - pool->start) >> sys_pagesize_shift;
        size = slab & ~SLAB_PAGE_START;

        slab_free_pages(pool, &pool->pages[n], size);

        return;
    }

    /* not reached */

    return;

done:

    return;

wrong_chunk:

    goto fail;

chunk_already_free:

	goto fail;

fail:

    return;
}

int g_slab_check(struct slab_pool_t *pool, void *p)
{
	uint8_t *u = (uint8_t *) p;
	return (u >= pool->start && u < pool->end);
}
/*--------------------------------------------------------------*/
#pragma pack(push, 1)

typedef struct fsb_pool_t
{
	long block_size;
	long block_count;
	volatile long begin;
	volatile long end;
	union {
		volatile uint32_t next_magic;
		unsigned long _spare;
	};
	long ids[1];
} fsb_pool_t;

typedef struct fsb_block
{
	union {
		uint64_t key;
		struct {
			int32_t	 id;
			uint32_t magic;
		};
	};
	uint8_t data[0];
} fsb_block;

#pragma pack(pop)

#define FS_BLOCK_POOL_OFFSET(p)  \
  (sizeof(fsb_pool_t) + sizeof(long) * p->block_count)
#define FS_GET_BLOCK_POOL(p)  \
  (((uint8_t*)(p)) + FS_BLOCK_POOL_OFFSET(p))
#define FS_GET_BLOCK_HEADER(p, id) \
  ((fsb_block*)(FS_GET_BLOCK_POOL(p) + (id) * p->block_size))
#define FS_GET_BLOCK_ID(p, header) \
  ((((uint8_t*)header) - FS_GET_BLOCK_POOL(p)) / p->block_size)
#define FS_GET_HEADER(data)  \
  ((fsb_block*)(((uint8_t*)(data)) - sizeof(fsb_block)))

long g_fsb_needed(long block_size, int block_count)
{
	return (block_size + sizeof(long) + sizeof(struct fsb_block))
		 * block_count + sizeof(struct fsb_pool_t);
}

long g_fsb_available(long total, long block_size)
{
	total -= sizeof(struct fsb_pool_t);
	return total/(block_size + sizeof(long) + sizeof(struct fsb_block));
}

struct fsb_pool_t *g_fsb_init(
	void *addr, long tatal, long block_size, int block_count)
{
	if (addr && (block_size > 0) && (block_count > 0) 
			&& (tatal >= g_fsb_needed(block_size, block_count)))
	{
		struct fsb_pool_t *pool = (struct fsb_pool_t *) addr;
		int i;
		pool->block_size = block_size + sizeof(fsb_block);
		pool->block_count = block_count;
		pool->begin = 0;
		pool->next_magic = 0;
		pool->end = block_count;
		
		for (i = 0; i < block_count; i ++)
		{
			pool->ids[i] = i;
			FS_GET_BLOCK_HEADER(pool, i)->key = 0;
		}

		return pool;
	}
	return 0;
}

long g_fsb_size(struct fsb_pool_t *pool)
{
	return pool->block_size - sizeof(fsb_block);
}

int g_fsb_count(struct fsb_pool_t *pool)
{
	return (int) pool->block_count;
}

int g_fsb_used(struct fsb_pool_t *pool)
{
	int l = pool->begin - pool->end;
	return (l > 0) ? (l - 1) : (pool->block_count + l);
}

void* g_fsb_alloc(struct fsb_pool_t *pool)
{
	long id = -1;
	uint32_t magic;

	long s = pool->block_count + 1;

	long b = pool->begin;
	long e = pool->end;
	if (b != e)
	{
		id = pool->ids[b];
		b ++;
		if (b == s)
			b = 0;
		pool->begin = b;

		magic = pool->next_magic + 1;
		if (magic == 0)
			magic = 1;
		pool->next_magic = magic;
	}

	if (id >= 0)
	{
		fsb_block *header = FS_GET_BLOCK_HEADER(pool, id);
		header->magic = magic;
		header->id = (int32_t)id;
		return header->data;
	}
	return NULL;
}

int g_fsb_free(struct fsb_pool_t *pool, void *block)
{
	fsb_block *header = FS_GET_HEADER(block);
	long id = FS_GET_BLOCK_ID(pool, block);
	if ((id >= 0) && (id < pool->block_count) 
			&& (id == (long)header->id) && (header->magic != 0))
	{
		long s = pool->block_count + 1;
		long e = pool->end;

		header->key = 0;
		pool->ids[e] = id;
		e ++;
		if (e == s)
			e = 0;
		pool->end = e;

		return 0;
	}
	return -1;
}

uint64_t g_fsb_getkey(struct fsb_pool_t *pool, const void *block)
{
	fsb_block *header = FS_GET_HEADER(block);
	long id = FS_GET_BLOCK_ID(pool, block);
	if ((id >= 0) && (id < pool->block_count) 
			&& (id == (long)header->id) && (header->magic != 0))
		return header->key;
	return 0;
}

void* g_fsb_getblock(struct fsb_pool_t *pool, uint64_t key)
{
	struct fsb_block *k = (struct fsb_block *) &key;
	if ((k->magic != 0) && (k->id >= 0) && (k->id < pool->block_count))
	{
		fsb_block *header = FS_GET_BLOCK_HEADER(pool, k->id);
		if (header->key == key)
			return FS_GET_BLOCK_HEADER(pool, k->id)->data;
	}
	return NULL;
}

//-----------------------------------------------------------------
struct g_xslab_t
{
	void** head;
	int32_t usable_amount;
	int32_t shrink_amount;
	int32_t alloc_size;
};

g_xslab_t* g_xslab_init(int32_t size)
{
	g_xslab_t* slab = (g_xslab_t*)malloc(sizeof(g_xslab_t));
	if (slab == NULL) return NULL;

	if (size < sizeof(void*)) size = sizeof(void*);

	slab->head = NULL;
	slab->usable_amount = 0;
	slab->shrink_amount = 0;
	slab->alloc_size = size;

	return slab;
}

void g_xslab_destroy(g_xslab_t* slab)
{
	while (slab->usable_amount > 0) {
		void* ptr = slab->head;
		slab->head = (void**)((slab->head)[0]);
		--(slab->usable_amount);
		free(ptr);
	}
	free(slab);
}

void* g_xslab_alloc(g_xslab_t* slab)
{
	if (slab->usable_amount > 0) {
		void* ptr = slab->head;
		slab->head = (void**)((slab->head)[0]);
		--(slab->usable_amount);
		return ptr;
	}

	return malloc(slab->alloc_size);
}

void g_xslab_free(g_xslab_t* slab, void* ptr)
{
	if (LIKELY(slab->shrink_amount == 0)) {
		((void**)ptr)[0] = slab->head;
		slab->head = (void**)ptr;
		++(slab->usable_amount);
	}
	else {
		free(ptr);
		--(slab->shrink_amount);
	}
}

void g_xslab_shrink(g_xslab_t* slab, double keep)
{
	assert(keep >= 0.0 && keep <= 1.0);

	int32_t tmp_usable_amount = slab->usable_amount;
	slab->shrink_amount = tmp_usable_amount - (int32_t) (tmp_usable_amount * keep);
}

int32_t g_xslab_alloc_size(g_xslab_t* slab)
{
	return slab->alloc_size;
}

int32_t g_xslab_usable_amount(g_xslab_t* slab)
{
	return slab->usable_amount;
}

int32_t g_xslab_shrink_amount(g_xslab_t* slab)
{
	return slab->shrink_amount;
}
