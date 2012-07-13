#ifndef _MEMPOOL_H_2011_
#define _MEMPOOL_H_2011_

#include "os_types.h"
#include <stdlib.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/** a: SLAB memory allocator */
void g_slab_setup(size_t page_size); /* call beforehand */

struct slab_pool_t *g_slab_init(void *addr, size_t total);
void *g_slab_alloc(struct slab_pool_t *pool, size_t size);
void  g_slab_free(struct slab_pool_t *pool, void *p);
int g_slab_check(struct slab_pool_t *pool, void *p);

/** b: fixed-size-block allocator */
long g_fsb_needed(long block_size, int block_count);
long g_fsb_available(long total, long block_size);

struct fsb_pool_t *g_fsb_init(
	void *addr, long total, long block_size, int block_count);
void* g_fsb_alloc(struct fsb_pool_t *pool);
int g_fsb_free(struct fsb_pool_t *pool, void *block);

long g_fsb_size(struct fsb_pool_t *pool); /* get block size */
int g_fsb_count(struct fsb_pool_t *pool); /* get block count */
int g_fsb_used(struct fsb_pool_t *pool); /* get blocks used */

uint64_t g_fsb_getkey(struct fsb_pool_t *pool, const void *block);
void* g_fsb_getblock(struct fsb_pool_t *pool, uint64_t key);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _MEMPOOL_H_2011_ */


