#ifndef __HTAB_H_QS_2010__
#define __HTAB_H_QS_2010__

/// @file htab.h
/// @author Qingshan Luo (qluo.cn@gmail.com)
/// @date 2010.3.1
/// @brief the simplest hash table, based on Christopher Clark's work

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct g_htab_t g_htab_t;

/// @brief create a hash table
/// @param minsize minimum initial size of hashtable
/// @param hashfn function for hashing keys
/// @param eqfn function for determining key equality
/// @return newly created hashtable or NULL on failure
g_htab_t *g_htab_create(uint32_t minsize,
	 uint32_t (*hashfn) (void *), int (*eqfn) (void*,void*));

/// @brief destroy a hash table
/// @param h the hashtable
void g_htab_destroy(struct g_htab_t *h);

/// @brief insert into hash table 
/// @param h the hashtable to insert into
/// @param k the key - does not claim ownership
/// @param v the value - does not claim ownership
/// @return  non-zero for successful insertion
///
/// This function will cause the table to expand if the insertion would take
/// the ratio of entries to table size over the maximum load factor.
///
/// This function does not check for repeated insertions with a duplicate key.
/// The value returned when using a duplicate key is undefined -- when
/// the hashtable changes size, the order of retrieval of duplicate key
/// entries is reversed.
/// If in doubt, remove before insert.
int g_htab_insert(g_htab_t *h, void *k, void *v);

/// @brief search over a hash table
/// @param h the hashtable to search
/// @param k the key to search for  - does not claim ownership
/// @return the value associated with the key, or NULL if none found
void *g_htab_search(g_htab_t *h, void *k);

/// @brief remove from a hash table
/// @param h the hashtable to remove the item from
/// @param k the key to search for  - does not claim ownership
/// @return the value associated with the key, or NULL if none found
void *g_htab_remove(struct g_htab_t *h, void *k);

/// @brief count the record number in a hash table
/// @param h the hashtable
/// @return the number of items stored in the hashtable
uint32_t g_htab_count(struct g_htab_t *h);


/// @brief traverse all data (k,v) pairs
/// @param h the hashtable
/// @param cb a callback function, give 0 if need to break
/// @param user the client data handler
void g_htab_foreach(struct g_htab_t *h, 
	int (*cb) (void*k, void*v, void *user), void *user);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif//__HTAB_H_QS_2010__
