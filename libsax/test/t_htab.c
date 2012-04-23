#include <sax/htab.h>

#include <stdio.h>
#include <string.h>

static uint32_t hashstr(void *x)
{
	uint32_t key = 97;
	char *s = (char *) x;
	while (*s) 
		key = 37*key + *s++;
	return key;
}

static int equstr(void *x, void *y)
{
	return 0 == strcmp((char *)x, (char *)y);
}

static int dumpstr(void *k, void *v, void *user)
{
	printf("k=%s  ---->   v=%s\n", (char *)k, (char *)v);
	return 1;
}

int main( int argc, char *argv[] )
{
	g_htab_t *h = g_htab_create(80, &hashstr, &equstr);
	g_htab_insert(h, "k001", "v001");
	g_htab_insert(h, "k002", "v002");
	g_htab_insert(h, "k003", "v003");
	g_htab_insert(h, "k004", "v004");
	g_htab_insert(h, "k005", "v005");
	g_htab_insert(h, "k006", "v006");
	g_htab_insert(h, "k007", "v007");
	g_htab_insert(h, "k008", "v008");
	g_htab_insert(h, "k009", "v009");
	printf("total=%d\n", g_htab_count(h));
	printf("k003: %s\n", (char *)g_htab_search(h, "k003"));
	printf("k006: %s\n", (char *)g_htab_search(h, "k006"));
	printf("k009: %s\n", (char *)g_htab_search(h, "k009"));
	g_htab_remove(h, "k007");
	g_htab_remove(h, "k009");
	printf("total=%d\n", g_htab_count(h));
	g_htab_foreach(h, &dumpstr, 0);
	g_htab_destroy(h);
	return 0;
}
