/**
 * @file bucket.h
 * @brief The header file of bucket_chain.
 * @author flylle
 *
 * This file defines class bucket_chain.
 */

#ifndef bucket_chain_H_
#define bucket_chain_H_

#include <stdio.h>
#include <iostream>

#include <sax/sysutil.h>
using namespace std;

/**
 *  \brief bucket_chain, represents a bucket of our array hash. 
 *   
 *   It has fixed size and an pointer to next bucket_chain element.
 *   It uses a char* member str to store item(binary) sequentially. When full, items will 
 *   be stored to next bucket_chain element.   
 *
 */

namespace sax {
class bucket_chain {
	size_t bucketSize_;
	g_share_t *filelock_;
public:
	char *str;
	long fpos;
	int num;
	bucket_chain* next;
	bool isLoaded;
	bool isDirty;
	long nextfpos;
	int level;
	
	
	/**
	 *  constructor
	 */
	bucket_chain(size_t bucketSize, g_share_t* fileLock):bucketSize_(bucketSize),filelock_(fileLock)
	{
		str = NULL;
		num = 0;
		next = 0;
		isLoaded = false;
		isDirty = true;
		fpos = 0;
		nextfpos = 0;
		level = 0;
	}

	/**	 
	 *  deconstructor
	 */
	virtual ~bucket_chain() {

		unload();
		if (next) {
			delete next;
			next = 0;
		}
		isLoaded = false;
	}

	/**
	 *  write to disk
	 */
	bool write(FILE* f);
	
	bool load(FILE* f) {
		bool ret = this->read_file(f);
		return ret;
	}
	
	/**
	 *  read from disk
	 */
	bool read_file(FILE* f);

	/**
	 *    load next bucket_chain element.
	 */
	bucket_chain* loadNext(FILE* f, bool& loaded) {
		loaded = false;
		if (next && !next->isLoaded) {
			next->read_file(f);
			loaded = true;
		}
		if (next)
			next->level = level+1;
		return next;
	}

	/**
	 *   unload a buck_chain element.
	 *   It releases most of the memory, and was used to recycle memory when cache is full. 
	 */
	bool unload() {
		if (str) {
			delete [] str;
			str = 0;
			isLoaded = false;
			return true;
		}
		isLoaded = false;
		return false;
	}

	/**
	 *   display string_chain info.
	 */
	void display(std::ostream& os = std::cout) {
		os<<"(level: "<<level;
		os<<"  isLoaded: "<<isLoaded;
		os<<"  bucketSize: "<<bucketSize_;
		os<<"  numitems: "<<num;
		os<<"  fpos: "<<fpos;
		if (next)
			os<<"  nextfpos: "<<next->fpos;
		os<<")- > ";
		if (next)
			next->display(os);
		os<<endl;
	}
};
}
#endif /*bucket_chain_H_*/
