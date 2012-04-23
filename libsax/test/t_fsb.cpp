#include <stdio.h>
#include <sax/mempool.h>
	
int main( int argc, char *argv[] )
{
	long total = g_fsb_needed(sizeof(int), 4);
	void *addr = malloc(total);
	struct fsb_pool_t *pool = g_fsb_init(addr, total, sizeof(int), 4);
	void *p1, *p2;
	printf( "%p, %p: %ld!\n", pool, addr, total);
	printf( "%ld, %d, %d!\n", g_fsb_size(pool), g_fsb_count(pool), g_fsb_used(pool));
	printf( "%p\n", p1=g_fsb_alloc(pool));
	printf( "%p\n", p2=g_fsb_alloc(pool));
	printf( "%p\n", g_fsb_alloc(pool));
	printf( "%ld, %d: %d!\n", g_fsb_size(pool), g_fsb_count(pool), g_fsb_used(pool));
	printf( "%p\n", g_fsb_alloc(pool));
	printf( "%p\n", g_fsb_alloc(pool));
	printf( "%ld, %d: %d!\n", g_fsb_size(pool), g_fsb_count(pool), g_fsb_used(pool));
	printf( "%d\n", g_fsb_free(pool, p1));
	printf( "%d\n", g_fsb_free(pool, p2));
	printf( "%d\n", g_fsb_free(pool, p1));
	printf( "%ld, %d: %d!\n", g_fsb_size(pool), g_fsb_count(pool), g_fsb_used(pool));
	free(addr);
	return 0;
}
