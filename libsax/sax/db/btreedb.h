
/// @file btreedb.h
/// @brief The header file of btreedb.
/// This file defines class btreedb.

#ifndef sax_btree_db_H_
#define sax_btree_db_H_

#include "btree_node.h"
#include "clocktimer.h"

#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <fstream>
#include <queue>
#include <map>

using namespace std;

namespace sax {

 /// @brief file version of cc-b*-btree
 /// A B*-tree is a tree data structure used in the HFS and Reiser4 file systems,
 /// which requires non-root nodes to be at least 2/3 full instead of 1/2. To maintain
 /// this, instead of immediately splitting up a node when it gets full, its keys are
 /// shared with the node next to it. When both are full, then the two of them
 /// are split into three. Now SDBv1.0¡¯s disk space is large than BerkeleyDB when
 /// storing the same data set. There is a perspective that, B*-tree can save more
 /// disk space than normal btree.
 /// For implementation convience and maintainess, we only apply this delay splitting
 /// at leaves nodes.
 /// Merging will occur when two sibling nodes' objCount both less than maxKeys/3/


static int key_cmp(const void *lhs, size_t lsz, const void *rhs, size_t rsz)
{
	size_t len = lsz > rsz ? rsz : lsz;
	int ret = memcmp(lhs, rhs, len);
	return ret == 0 ? lsz - rsz : ret;
}


class btreedb
{
	enum {unloadbyRss = false};
	enum {unloadAll = true};
	enum {orderedCommit =true};
	enum {quickFlush = false};
public:
	typedef std::pair<btree_node*, size_t> Cursor;
	
	struct DataPack
	{
		size_t klen;
		void * key;
		size_t vlen;
		void * val;
		DataPack():klen(0),key(NULL),vlen(0),val(NULL){}
	};
	
public:

	 /// @brief constructor
	 /// @param fileName is the name for data file if fileName ends with '#', we set b-tree mode to
	 /// not delay split.
	btreedb(const std::string& fileName = "btreedb.dat", bool thread_safe = false, cmpfunc  *comp = &key_cmp);
	virtual ~btreedb();

	/// @brief open the database.
	/// Everytime  we use the database, we mush open it first.
	bool open();
	bool is_open();

	void clear();
	
	///  @brief write back the dirypages
	void commit();
	
	///  @brief write all the items in memory to file.
	void flush();
	
	/// @brief close the database.
	///  if we don't call it, it will be automately called in deconstructor
	bool close() ;

	/// @brief insert an item.
	bool insert(const void *key, size_t klen, const void  *value, size_t vlen);

	/// @brief find an item given a key.
	/// retval shall be delete!!
	void *find(const void *key, size_t klen, size_t &vlen);
	size_t find(const void *key, size_t klen, void *val);
	
	/// the same as find
	bool get(const void  *key, size_t klen, void *value, size_t &vlen);

	/// @brief find an item given a key.
	const void *find2(const void  *key, size_t klen, size_t & vlen);

	/// @brief updata an item with given key, if it not exist, insert it directly.
	bool update(const void *key, size_t klen, const void *val, size_t vlen);
	
	/// @brief del an item from the database
	bool del(const void *key, size_t klen);
	
	 /// @brief get the cursor for given key
	 /// @param locn is cursor of key.
	 /// @return true if key exists otherwise false.
	bool search(const void *key, size_t klen, Cursor& locn) ;


	/// @brief get an item from given Locn.
	bool get(const Cursor& locn, void *key, size_t &klen, void *value, size_t &vlen);

	/// @brief get the cursor of the first item.
	Cursor get_first_locn();
	Cursor get_last_locn();
	
	bool range(size_t start, size_t end, std::vector<DataPack> &rr);

	/// @brief get the next or prev item.
	/// @locn when locn is default value, it will start with firt element when sdri=ESD_FORWARD
	/// and start with last element when sdir = ESD_BACKWARD
	bool seq(Cursor& locn, void *key, size_t klen, void *value, size_t vlen, bool sdir = true);
	bool seq(Cursor& locn, bool sdir=true);
	
	/// for debug.  print the shape of the B tree.
	void display(std::ostream& os = std::cout, bool onlyheader = false) ;
	
	/// @brief set mod
	/// @param delaySplit, if true, the btree is cc-b*-btee, otherwise is normal cc-b-tree.
	/// For ascending insertion, cc-b-tree is much faster than cc-b*-btree, while cc-b*-btree
	/// uses less disk space and find faster than cc-b-btree.
	void setBtreeMode(bool autoTuning=true, unsigned int optimizeNum = 65536, bool delaySplit=true);
	
	/// @brief set the MaxKeys
	/// Note that it must be at least 6.
	/// It can only be called before open. And it doesn't work when open an existing dat file,
	/// _sfh.maxKeys will be read from file.
	void setMaxKeys(size_t maxkeys);

	/// @brief set maxKeys for fileHeader
	/// maxKeys is 2*degree
	/// Note that it must be at least 6
	/// It can only be called  before opened.And it doesn't work when open an existing dat file,
	/// _sfh.maxKeys will be read from file.
	void setDegree(size_t degree);
	
	 ///  @brief set tha pageSize of fileHeader.
	 /// It can only be called before open.And it doesn't work when open an existing dat file,
	 /// _sfh.pageSize will be read from file.
	 /// It should set the pageSize according the maxKeys and max inserting data size.
	 /// When pageSize is too small, overflowing will occur and cause efficiency to decline.
	 /// When pageSize is too large, it will waste disk space.
	void setPageSize(size_t pageSize);

	void selfCheck()
	{
		size_t ll = 0;
		ll += sizeof(unsigned char); //isleaf
		ll += (sizeof(size_t)*3);
		ll += ((_sfh.maxKeys+3)*sizeof(long));
		if(_sfh.pageSize < ll)
			_sfh.pageSize = ll;
	}

	 /// @brief set Cache Size.
	 /// Cache Size is the active node number in memory.When cache is full,
	 /// some nodes will be released.
	 /// We would peroidically flush the memory items, according to the cache Size.
	void setCacheSize(size_t sz);
	
	 /// @brief set file name.
	void setFileName(const std::string& fileName );
	
	 ///  @brief return the file name of the SequentialDB
	std::string getFileName() const ;
	
	/// @brief get the number of the items.
	int num_items();

	template<typename AM>
	bool dump(AM& other);

	bool dump2f(const string& fileName);
	
	btree_node *getRoot() ;

	void optimize();
	
	void fillCache() ;
	
	bool seq_(Cursor& locn, bool sdir=true);
	
	bool search_(const void *key, size_t klen, Cursor& locn);
	
private:
	btree_node *_root;
	FILE *_dataFile;
	CbFileHeader _sfh;
	size_t _cacheSize;

	bool _isDelaySplit;
	bool _isOpen;
	bool _autoTunning;
	unsigned int _optimizeNum;

//	izenelib::am::CompareFunctor<KeyType> _comp;
	cmpfunc *_comp;
	std::string _fileName; // name of the database file
private:
	g_share_t *_fileLock;
	g_share_t *_flushLock;
	size_t _activeNodeNum;
	size_t _dirtyPageNum;

private:
	unsigned long _initRss;
	unsigned int _flushCount;

	inline void _setDirty(btree_node *node);

	void _flushCache() ;

	void _safeFlushCache() ;

//for seq, reset Cursor
	void _flushCache(Cursor& locn) ;

	void _flushCacheImpl(bool quickFlush=false);

	btree_node *_allocateNode() ;

	void _split(btree_node *parent, size_t childNum, btree_node*child);
	void _split3Leaf(btree_node *parent, size_t childNum);
	btree_node *_merge(btree_node *parent, size_t objNo);

	bool _seqNext(Cursor& locn);
	bool _seqPrev(Cursor& locn);
	void _flush(btree_node *node, FILE* f);
	bool _delete(btree_node *node, const void*key, size_t klen);

	// Finds the location of the predecessor of this key, given
	// the root of the subtree to search. The predecessor is going
	// to be the right-most object in the right-most leaf node.
	Cursor _findPred(btree_node *node) ;

	// Finds the location of the successor of this key, given
	// the root of the subtree to search. The successor is the
	// left-most object in the left-most leaf node.
	Cursor _findSucc(btree_node *node) ;

	void optimize_(bool autoTuning = false) ;
	
	bool packData(btree_node *node, size_t it, DataPack &p);

};

}
#endif /*btreedb_H_*/
