#ifndef __HASH_DB_2011__
#define __HASH_DB_2011__

#include <string>
#include <queue>
#include <map>
#include <iostream>
#include <sys/stat.h>
#include <sys/time.h>

#include <assert.h>
#include <stdio.h>

#include "hash_header.h"
#include "hash_bucket.h"

namespace sax {

static uint32_t sdb_hash_fun(const void* kbuf, const size_t ksize) {
	uint32_t convkey = 0;
	const char* str = (const char*)kbuf;
	for (size_t i = 0; i < ksize; i++)
	convkey = 37*convkey + (uint8_t)*str++;
	return convkey;
}


class hashdb
{
	enum {unloadByRss = false}; //unused
	enum {unloadAll = false};  //false
	enum {orderedCommit =true}; //true
	enum {delayFlush = true};  //true
	enum {quickFlush = false}; //unused
	
public:

	typedef uint32_t (hashfunc_t)(const void *key, const size_t ksize);
	typedef std::pair<bucket_chain*, char*> Cursor;

	hashdb(const string fileName, bool thread_safe = false, hashfunc_t *func = &sdb_hash_fun);

	/// deconstructor, close() will also be called here.
	virtual ~hashdb();
	
	/// db must be opened to be used.
	bool open() ;
	
	/// check if it is open
	bool is_open();

	/// db should be closed after open, and  it will automatically called in deconstuctor.
	bool close();
	
	/// Write the dirty buckets to disk and also free up most of the memory.
	/// Note that, for efficieny, entry_[] is not freed up.
	void flush();
	
	/// write the dirty buckets to disk, not release the memory
	void commit() ;
	
	/// insert an item
	bool insert(const void *key, size_t ksz, const void *val, size_t vsz);
	
	/// find an item, return pointer to the value.
	/// Note that, there will be memory leak if not delete the value
	size_t find(const void * key, size_t ksize, void *val) ;
	
	/// delete an item
	bool del(const void * key, size_t ksize);

	/// update  an item by key/value pair
	bool update(const void *key, size_t ksz, const void *val, size_t vsz);
	
	/// another search function, flushCache_() will be called at the beginning
	bool search(const void *key, size_t ksize, Cursor &locn);

	/// get the Cursor of first item in the first not empty bucket.
	Cursor get_first_locn();
	
	/// @brief sequential access method
	/// @param locn is the current Cursor, and will replaced next Cursor when route finished.
	/// @param sdir is sequential access direction, for hash is unordered, we only implement forward case.
	bool seq(Cursor& locn, void * key, size_t &ksz, void * value, size_t &vsz, bool sdir = true);
	
	bool seq(Cursor& locn, bool sdir = true) ;
	
	/// get the current key/val from cursor
	bool get(const Cursor& locn, void *key, size_t &ksize, void * val, size_t &vsize);
	
	/// get the num of items
	int num_items() ;
	
	template<typename AM>
	bool dump(AM& other);

	bool dump2f(const string& fileName);
	
	void optimize();

	
private:
	
	bool insert_(const void *key, size_t ksz, const void *val, size_t vsz);
	
	size_t find_(const void * key, size_t ksize, void *val) ;
	
	bool del_(const void * key, size_t ksize);
	
	bool update_(const void *key, size_t ksz, const void *val, size_t vsz);
	
	bool search_(const void *key, size_t ksize, Cursor &locn);
	
	bool get_(const Cursor& locn, void *key, size_t &ksize, void * val, size_t &vsize);

	bool seq_(Cursor& locn);
	
	void optimize_();
	
public:
	
	/// display the info of sdb_hash
	void display(std::ostream& os = std::cout, bool onlyheader = true);

	/// @brief It displays how much space has been wasted in percentage after deleting or updates.
	/// when an item is deleted, we don't release its space in disk but set a flag that
	/// it have been deleted. And it will lead to low efficiency. Maybe we should dump it
	/// to another files when loadFactor are low.
	double loadFactor() ;


public:

	inline void sethash(hashfunc_t *func) { hash_fun = func;}

	void fillCache();
	
	/// clear the db
	void clear() ;
	
	/// set cache size, if not called use default size 100000
	/// this function can be called at anytime
	void setCacheSize(size_t cacheSize);
	
	
	/// @brief set bucket size of fileHeader
	/// if not called use default size 8192
	/// before open
	void setBucketSize(size_t bucketSize);

	/// @brief set bucket size of fileHeader
	/// if not called use default size 8192
	/// before open
	void setPageSize(size_t pageSize);

	/// set directory size if fileHeader
	/// if not called use default size 4096
	/// before open
	void setDegree(size_t dpow);
	
	/// @brief set file name.
	void setFileName(const std::string& fileName );

	/// @brief return the file name of the SequentialDB
	std::string getFileName() const;
	
	
protected:

	hashfunc_t *hash_fun;
	bucket_chain** entry_;

	/// bucketAddr stores fpos for entry_ and it was store in disk after fileHeader.
	long *bucketAddr;

	/// levle->bucket_chain* map, used for caching
	multimap<int, bucket_chain*, greater<int> > sh_cache_;
	typedef multimap<int, bucket_chain*, greater<int> >::iterator CacheIter;

private:
	hash_header sfh_;
	string fileName_;
	FILE* dataFile_;
	bool isOpen_;

	unsigned int activeNum_;
	unsigned int dirtyPageNum_;
	
	bool _thread_safe;
	g_share_t *_file_lock;
	g_share_t *_flush_lock;
	
private:
	size_t directorySize_;
	size_t dmask_;
	size_t cacheSize_;

	size_t ksize_;
	size_t vsize_;
	size_t BucketGap;

	unsigned int flushCount_;

	void setDirty_(bucket_chain* bucket) ;

	bool load_(bucket_chain* bucket);
	
	/// Allocate an bucket_chain element
	bucket_chain* allocateBlock_() ;

	bucket_chain* loadNext_(bucket_chain* current);
	
	bool isEmptyBucket_(uint32_t idx, bucket_chain* &sa);

	/// when cache is full, it was called to reduce memory usage.
	void safeFlushCache_();
	
	void flushCache_();

	void safeFlushCache_(Cursor &locn);

	void flushCacheImpl_();
	
	void unload_();

};

}
#endif//__HASH_DB_2011__
