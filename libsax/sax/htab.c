#include "htab.h"

#include <stdlib.h>
#include <string.h>

struct entry_t {
    void *k, *v;
    uint32_t h;
    struct entry_t *next;
};

struct g_htab_t {
	struct entry_t **table;
    uint32_t tablelength; 
    uint32_t entrycount;
    uint32_t loadlimit;
    uint32_t primeindex;
    uint32_t (*hashfn) (void *k);
    int (*eqfn) (void *k1, void *k2);
};

/* Aim to protect against poor hash functions by adding
   logic taken from java 1.4 hashtable source */
static uint32_t do_hash(struct g_htab_t *h, void *k)
{
    uint32_t i = h->hashfn(k);
    i += ~(i << 9);
    i ^=  ((i >> 14) | (i << 18));
    i +=  (i << 4);
    i ^=  ((i >> 10) | (i << 22));
    return i;
}

/* Credit for primes table: Aaron Krowne
   http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
   http://br.endernet.org/~akrowne/  */
static const uint32_t primes[] = {
	53, 97, 193, 389,
	769, 1543, 3079, 6151,
	12289, 24593, 49157, 98317,
	196613, 393241, 786433, 1572869,
	3145739, 6291469, 12582917, 25165843,
	50331653, 100663319, 201326611, 402653189,
	805306457, 1610612741
};
const uint32_t prime_table_length = sizeof(primes)/sizeof(primes[0]);
const float max_load_factor = 0.65;

struct g_htab_t *g_htab_create(uint32_t minsize,
	uint32_t (*hashfn) (void*), int (*eqfn) (void*,void*))
{
    struct g_htab_t *h;
    uint32_t pindex, size = primes[0];
    
    /* Check requested hashtable isn't too large */
    if (minsize > (1u << 30)) return NULL;
    
    /* Enforce size as prime */
    for (pindex=0; pindex < prime_table_length; pindex++) {
        if (primes[pindex] > minsize) { size = primes[pindex]; break; }
    }
    
    h = (struct g_htab_t *)malloc(sizeof(struct g_htab_t));
    if (NULL == h) return NULL;
    
    h->table = (struct entry_t **)calloc(size, sizeof(struct entry_t*));
    if (NULL == h->table) { free(h); return NULL; }
    
    h->tablelength = size;
    h->primeindex  = pindex;
    h->entrycount  = 0;
    h->hashfn      = hashfn;
    h->eqfn        = eqfn;
    h->loadlimit   = (uint32_t) (size * max_load_factor + 0.5f);
    return h;
}

static int htab_expand(struct g_htab_t *h)
{
    /* Double the size of the table to accomodate more entries */
    struct entry_t **newtable;
    struct entry_t *e;
    struct entry_t **pE;
    uint32_t newsize, i, index;
    
    /* Check we're not hitting max capacity */
    if (h->primeindex == (prime_table_length - 1)) return 0;
    newsize = primes[++(h->primeindex)];

    newtable = (struct entry_t **)malloc(sizeof(struct entry_t*) * newsize);
    if (NULL != newtable)
    {
        memset(newtable, 0, newsize * sizeof(struct entry_t *));
        /* This algorithm is not 'stable'. ie. it reverses the list
         * when it transfers entries between the tables */
        for (i = 0; i < h->tablelength; i++) {
            while (NULL != (e = h->table[i])) {
                h->table[i] = e->next;
                index = (e->h % newsize);
                e->next = newtable[index];
                newtable[index] = e;
            }
        }
        free(h->table);
        h->table = newtable;
    }
    /* Plan B: realloc instead */
    else 
    {
        newtable = (struct entry_t **)
                   realloc(h->table, newsize * sizeof(struct entry_t *));
        if (NULL == newtable) { (h->primeindex)--; return 0; }
        h->table = newtable;
        memset(newtable[h->tablelength], 0, newsize - h->tablelength);
        for (i = 0; i < h->tablelength; i++) {
            for (pE = &(newtable[i]), e = *pE; e != NULL; e = *pE) {
                index = (e->h % newsize);
                if (index == i)
                {
                    pE = &(e->next);
                }
                else
                {
                    *pE = e->next;
                    e->next = newtable[index];
                    newtable[index] = e;
                }
            }
        }
    }
    h->tablelength = newsize;
    h->loadlimit   = (uint32_t) (newsize * max_load_factor + 0.5f);
    return -1;
}

int g_htab_insert(struct g_htab_t *h, void *k, void *v)
{
    /* This method allows duplicate keys - but they shouldn't be used */
    uint32_t index;
    struct entry_t *e;
    if (++(h->entrycount) > h->loadlimit)
    {
        /* Ignore the return value. If expand fails, we should
         * still try cramming just this value into the existing table
         * -- we may not have memory for a larger table, but one more
         * element may be ok. Next time we insert, we'll try expanding again.*/
        (void) htab_expand(h);
    }
    e = (struct entry_t *)malloc(sizeof(struct entry_t));
    if (NULL == e) { --(h->entrycount); return 0; }
    
    e->h = do_hash(h,k);
    index = (e->h % h->tablelength);
    e->k = k;
    e->v = v;
    e->next = h->table[index];
    h->table[index] = e;
    return -1;
}

void *g_htab_search(struct g_htab_t *h, void *k)
{
    struct entry_t *e;
    uint32_t hashvalue, index;
    hashvalue = do_hash(h,k);
    index = (hashvalue % h->tablelength);
    e = h->table[index];
    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (h->eqfn(k, e->k))) return e->v;
        e = e->next;
    }
    return NULL;
}

void *g_htab_remove(struct g_htab_t *h, void *k)
{
    /* TODO: consider compacting the table when the load factor drops enough,
     *       or provide a 'compact' method. */

    struct entry_t *e;
    struct entry_t **pE;
    void *v;
    uint32_t hashvalue, index;

    hashvalue = do_hash(h,k);
    index = (hashvalue % h->tablelength);
    pE = &(h->table[index]);
    e = *pE;
    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (h->eqfn(k, e->k)))
        {
            *pE = e->next;
            h->entrycount--;
            v = e->v;
            free(e);
            return v;
        }
        pE = &(e->next);
        e = e->next;
    }
    return NULL;
}

void g_htab_destroy(struct g_htab_t *h)
{
    uint32_t i;
    struct entry_t *e, *f;
    struct entry_t **table = h->table;
    for (i = 0; i < h->tablelength; i++)
    {
        e = table[i];
        while (NULL != e) {
        	f = e; e = e->next; free(f);
        }
    }
    free(h->table);
    free(h);
}

uint32_t g_htab_count(struct g_htab_t *h)
{
    return h->entrycount;
}

void g_htab_foreach(struct g_htab_t *h, 
	int (*cb) (void*k, void*v, void *user), void *user)
{
    uint32_t i;
    struct entry_t *e, *f;
    struct entry_t **table = h->table;
    for (i = 0; i < h->tablelength; i++)
    {
        e = table[i];
        while (NULL != e) {
        	f = e; e = e->next; 
        	if (0 == cb(f->k, f->v, user)) return;
        }
    }
}

