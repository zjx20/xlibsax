
#include "hashdb.h"
using namespace std;
using namespace sax;

class ClockTimer
{
public:
    /// @brief remembers start time during construction
    ClockTimer()
    : start_()
    {
        gettimeofday(&start_, 0);
    }

    /// @brief resets start time to current time.
    void restart()
    {
        gettimeofday(&start_, 0);
    }
    
     /// @brief returns elapsed seconds since remembered start time.
     /// @return elapsed time in seconds, some platform can returns fraction
     /// representing more precise time.
    double elapsed() const
    {
        timeval now;
        gettimeofday(&now, 0);

        double seconds = now.tv_sec - start_.tv_sec;
        seconds += (now.tv_usec - start_.tv_usec) / 1000000.0;

        return seconds;
    }

private:
    timeval start_; /**< remembers start time */
};


hashdb::hashdb(const string fileName , bool thread_safe, hashfunc_t *func):sfh_(), fileName_(fileName) {
	directorySize_ = (1<<sfh_.dpow);
	dmask_ = directorySize_ - 1;
	dataFile_ = 0;
	isOpen_ = false;
	activeNum_ = 0;
	dirtyPageNum_ = 0;
	cacheSize_ = 0;
	ksize_ = vsize_ = 0;
	hash_fun = func;
	_thread_safe = thread_safe;
	if(_thread_safe){
		_file_lock = g_share_init();
		_flush_lock = g_share_init();
	}
	else{
		_file_lock = _flush_lock = NULL;
	}
}

hashdb::~hashdb() {
	if(dataFile_)
	close();
	if(_thread_safe)
	{
		g_share_free(_file_lock);
		g_share_free(_flush_lock);
	}
}

void hashdb::clear() {
	close();
	std::remove(fileName_.c_str() );
	sfh_.initialize();
	open();
}

bool hashdb::is_open() {
	return isOpen_;
}


void hashdb::setBucketSize(size_t bucketSize) {
	assert(isOpen_ == false);
	sfh_.bucketSize = bucketSize;
}

/**
 *  \brief set bucket size of fileHeader
 *
 *   if not called use default size 8192
 */
void hashdb::setPageSize(size_t pageSize) {
	setBucketSize( pageSize );
}


void hashdb::setDegree(size_t dpow) {
	assert(isOpen_ == false);
	sfh_.dpow = dpow;
	directorySize_ = (1<<dpow);
	dmask_ = directorySize_ - 1;

}

void hashdb::setCacheSize(size_t cacheSize)
{
	sfh_.cacheSize = cacheSize;
	cacheSize_ = cacheSize;
}

void hashdb::setFileName(const std::string& fileName ) {
	fileName_ = fileName;
}

std::string hashdb::getFileName() const {
	return fileName_;
}

bool hashdb::insert(const void * key, size_t ksize, const void *value, size_t vsize)
{
	share_rwlock lock(_flush_lock, 0);
	return insert_(key, ksize, value, vsize);
}

bool hashdb::insert_(const void * key, size_t ksize, const void *value, size_t vsize) {
	if( !isOpen_ ) return false;
	flushCache_();
	Cursor locn;
	if( search_(key, ksize, locn) ) return false;
	else {
		bucket_chain* sa = locn.first;
		char* p = locn.second;

		if(locn.first == NULL) {
			uint32_t idx = hash_fun(key, ksize) & dmask_;

			if (bucketAddr[idx] == 0) {
				entry_[idx] = allocateBlock_();
				bucketAddr[idx] = entry_[idx]->fpos;

				sa = entry_[idx];
				p = entry_[idx]->str;
			}
		}
		else
		{
			assert(locn.second != NULL);
			
			BucketGap = ksize+vsize + sizeof(long)+sizeof(int) +3*sizeof(size_t);

			//add an extra size_t to indicate if reach the end of  bucket_chain.
			if ( size_t(p - sa->str)> sfh_.bucketSize-BucketGap ) {
				if (sa->next == 0) {
					setDirty_(sa);
					sa->next = allocateBlock_();
				}
				sa = loadNext_( sa );
				p = sa->str;
			}
		}

		memcpy(p, &ksize, sizeof(size_t));
		p += sizeof(size_t);
		memcpy(p, &vsize, sizeof(size_t));
		p += sizeof(size_t);
		memcpy(p, key, ksize);
		p += ksize;
		memcpy(p, value, vsize);
		p += vsize;

		assert( size_t (p-sa->str) + sizeof(long) + sizeof(int) <= sfh_.bucketSize);
		setDirty_(sa);
		++sa->num;
		++sfh_.numItems;
		return true;
	}
}

size_t hashdb::find(const void * key, size_t ksize, void *val)
{
	share_rwlock lock(_flush_lock, 0);
	return find_(key, ksize, val);
}

size_t hashdb::find_(const void * key, size_t ksize, void *val) {
	if( !isOpen_ )
	return -1;

	//ScopedReadLock<LockType> lock(flushLock_);
	Cursor locn;
	size_t vsz = -1;
	if( !search_(key, ksize, locn) ) return -1;
	char *p = locn.second;

	p += sizeof(size_t);
	memcpy(&vsz, p, sizeof(size_t));
	p += sizeof(size_t);
	p += ksize;
	memcpy(val, p, vsz);
	return vsz;
}

bool hashdb::del(const void * key, size_t ksize)
{
	share_rwlock lock(_flush_lock, 1);
	return del_(key, ksize);
}

bool hashdb::del_(const void * key, size_t ksize) {
	if( !isOpen_ )
		return false;
	
	Cursor locn;
	if( search_(key, ksize, locn) )
	{
		char *p = locn.second;
		size_t ksz, vsz;
		memcpy(&ksz, p, sizeof(size_t));
		p += sizeof(size_t);
		memcpy(&vsz, p, sizeof(size_t));
		p += sizeof(size_t);
		size_t leftSize = sfh_.bucketSize-(2*sizeof(size_t)+ksz+vsz)-(p-locn.first->str);
		memcpy(p-2*sizeof(size_t), p+ksz+vsz, leftSize);
		--locn.first->num;
		setDirty_(locn.first);
		--sfh_.numItems;
		return true;
	}
	else return false;
}

bool hashdb::update(const void * key, size_t ksize, const void * value, size_t vsize)
{
	share_rwlock lock(_flush_lock, 1);
	return update_(key, ksize, value, vsize);
}
	
bool hashdb::update_(const void * key, size_t ksize, const void * value, size_t vsize) {
	if( !isOpen_ ) return false;
	Cursor locn;
	
	if( !search_(key, ksize, locn) )
		return insert_(key, ksize, value, vsize);
	else
	{
		bucket_chain* sa = locn.first;
		char *p = locn.second;
		size_t ksz, vsz;
		memcpy(&ksz, p, sizeof(size_t));
		p += sizeof(size_t);
		memcpy(&vsz, p, sizeof(size_t));
		p += sizeof(size_t);
		if(vsz == vsize)
		{
			setDirty_(sa);
			memcpy(p+ksz, value, vsz);
			return true;
		}
		else
		{
			setDirty_(sa);
			memset(p, 0xff, ksize);
			return insert_(key, ksize, value, vsize);
		}

		return true;
	}
}

bool hashdb::search(const void *key, size_t ksize, hashdb::Cursor &locn) {
	safeFlushCache_();
	//ScopedReadLock<LockType> lock(flushLock_);
	share_rwlock lock(_flush_lock, 0);
	return search_(key, ksize, locn);
}

bool hashdb::search_(const void *key, size_t ksize, hashdb::Cursor &locn)
{
	if( !isOpen_ ) return false;

	locn.first = NULL;
	locn.second = NULL;

	uint32_t idx = hash_fun(key, ksize) & dmask_;

	if( entry_ == NULL )
	return false;
	locn.first = entry_[idx];

	if( !entry_[idx] )
	{
		if( bucketAddr[idx] != 0 ) {
			entry_[idx] = new bucket_chain(sfh_.bucketSize, _file_lock);
			entry_[idx] ->fpos = bucketAddr[idx];
		}
	}

	if (entry_[idx] == NULL) {
		return false;
	} else {
		int i = 0;
		bucket_chain* sa = entry_[idx];
		load_( sa );
		char* p = sa->str;
		while ( sa ) {
			locn.first = sa;
			p = sa->str;

			for (i=0; i<sa->num; i++)
			{
				size_t j=0;
				size_t ksz, vsz;
				memcpy(&ksz, p, sizeof(size_t));
				p += sizeof(size_t);
				memcpy(&vsz, p, sizeof(size_t));
				p += sizeof(size_t);

				if (ksz != ksize) {
					p += ksz + vsz;
					continue;
				}

				for (; j<ksz; j++) {
					if (((char*)key)[j] != p[j]) {
						break;
					}
				}

				if (j == ksz) {
					locn.second = p-2*sizeof(size_t);
					return true;
				}
				p += ksz + vsz;
			}
			sa = loadNext_(sa);
		}
		locn.second = p;
	}
	return false;
}

hashdb::Cursor hashdb::get_first_locn()
{
//		ScopedReadLock<LockType> lock(flushLock_);
	share_rwlock lock(_flush_lock, 0);

	hashdb::Cursor locn;
	locn.first = NULL;
	locn.second = NULL;
	if( sfh_.numItems == 0 )
	return locn;

	for(size_t i=0; i<directorySize_; i++)
	{
		if( !entry_[i] )
		{
			if( bucketAddr[i] != 0 ) {
				entry_[i] = new bucket_chain(sfh_.bucketSize, _file_lock);
				entry_[i] ->fpos = bucketAddr[i];
			}
		}
		if( entry_[i] )
		{
			load_( entry_[i] );
			locn.first = entry_[i];
			locn.second = entry_[i]->str;
			break;
		}
	}

	size_t ksz = 1024;
	size_t vsz = sfh_.bucketSize - sizeof(int) - sizeof(long);
	char * key = (char*)malloc(ksz);
	char *val = (char *)malloc(vsz);

	
	while( !get_(locn, key, ksz, val, vsz))seq_(locn);
	
	free(key);
	free(val);
	
	return locn;
}


bool hashdb::get(const Cursor& locn, void *key, size_t &ksize, void * val, size_t &vsize)
{
//	ScopedReadLock<LockType> lock(flushLock_);
	share_rwlock lock(_flush_lock, 0);
	return get_(locn, key, ksize, val, vsize);
}

bool hashdb::get_(const Cursor& locn, void *key, size_t &ksize, void * val, size_t &vsize)
{
	if( !isOpen_ )
	return false;

	bucket_chain* sa = locn.first;
	char* p = locn.second;

	if(sa == NULL)return false;
	if(p == NULL)return false;
	if(sa->num == 0)return false;

	size_t ksz, vsz;
	memcpy(&ksz, p, sizeof(size_t));
	p += sizeof(size_t);
	memcpy(&vsz, p, sizeof(size_t));
	p += sizeof(size_t);
	
	ksize = ksz;
	vsize = vsz;
	
	memcpy(key, p, ksize);
	p += ksize;
	memcpy(val, p, vsize);
	p += vsize;
	return true;

}

bool hashdb::seq(Cursor& locn, void * key, size_t &ksz, void * value, size_t &vsz, bool sdir)// util::ESeqDirection sdir=util::ESD_FORWARD)
{
	bool ret = seq(locn, sdir);
	get(locn, key, ksz, value, vsz);
	return ret;
}

bool hashdb::seq(Cursor& locn, bool sdir) {
	safeFlushCache_(locn);
	share_rwlock lock(_flush_lock, 0);
//		ScopedReadLock<LockType> lock(flushLock_);
	return seq_(locn);
}

bool hashdb::seq_(Cursor& locn) {
	if( !isOpen_ )
		return false;

	bucket_chain* sa = locn.first;
	char* p = locn.second;

	if(sa == NULL)return false;
	if(p == NULL)return false;

	char* ptr = 0;
	size_t poff = 0;

	size_t ksize = 0, vsize;
	if( sa->num != 0) {
		memcpy(&ksize, p, sizeof(size_t));
		p += sizeof(size_t);
		memcpy(&vsize, p, sizeof(size_t));
		p += sizeof(size_t);

		ptr = p;
		poff = ksize;
		p += ksize;
		p += vsize;
		memcpy(&ksize, p, sizeof(size_t));
	}
	if( ksize == 0 ) {
		sa = loadNext_(sa);
		while( sa && sa->num <= 0) {
			sa = loadNext_(sa);
		}
		if( sa ) {
			p = sa->str;
		}
		else
		{
			uint32_t idx = ptr ? hash_fun(ptr, poff) & dmask_ : -1;

			while( isEmptyBucket_(++idx, sa ) ) {
				if( idx >= directorySize_ -1) {
					break;
				}
			}

			if( sa ) p = sa->str;
			else
			p = NULL;
		}
	}


	locn.first = sa;
	locn.second = p;
	return true;
}

/**
 *   get the num of items
 */
int hashdb::num_items() {
	return sfh_.numItems;
}

void hashdb::fillCache() {
	if( !isOpen_ )
	return;

	if(sfh_.numItems == 0)
	return;

	typedef map<long, bucket_chain*> COMMIT_MAP;
	typedef COMMIT_MAP::iterator CMIT;
	COMMIT_MAP toBeRead, nextRead;
	for (size_t i=0; i<directorySize_; i++) {
		if( entry_&& entry_[i] && entry_[i]->fpos !=0 )
		toBeRead.insert(make_pair(entry_[i]->fpos, entry_[i]));
	}
	while( !toBeRead.empty() ) {
		CMIT it = toBeRead.begin();
		for (; it != toBeRead.end(); it++) {
			load_( it->second);
			if( activeNum_> sfh_.cacheSize )
				return;
			if(it->second->next)
				nextRead.insert(make_pair(it->second->nextfpos, it->second->next) );
		}
		toBeRead = nextRead;
		nextRead.clear();
	}
}

bool hashdb::open() {

	if(isOpen_) return true;
	struct stat statbuf;
	bool creating = stat(fileName_.c_str(), &statbuf);

	dataFile_ = fopen(fileName_.c_str(), creating ? "w+b" : "r+b");
	if ( 0 == dataFile_) {
		cout<<"Error in open(): open file failed"<<endl;
		return false;
	}
	bool ret = false;
	if (creating) {

		// We're creating if the file doesn't exist.
#ifdef DEBUG			
		cout<<"creating hashdb: "<<fileName_<<"...\n"<<endl;
		sfh_.display();
#endif
		bucketAddr = new long[directorySize_];
		entry_ = new bucket_chain*[directorySize_];

		//initialization
		memset(bucketAddr, 0, sizeof(long)*directorySize_);
		memset(entry_ , 0, sizeof(bucket_chain*)*directorySize_);
		commit();
		ret = true;
	} else {
		if ( !sfh_.fromFile(dataFile_) ) {
			return false;
		} else {
			if (sfh_.magic != 0x1a9999a1) {
				cout<<"Error, read wrong file header_\n"<<endl;
				return false;
			}
			if(cacheSize_ != 0)
			sfh_.cacheSize = cacheSize_;
#ifdef DEBUG
			cout<<"open exist sdb_hash: "<<fileName_<<"...\n"<<endl;
			sfh_.display();
#endif
			directorySize_ = (1<<sfh_.dpow);
			dmask_ = directorySize_ - 1;

			bucketAddr = new long[directorySize_];
			entry_ = new bucket_chain*[directorySize_];
			memset(bucketAddr, 0, sizeof(long)*directorySize_);
			memset(entry_ , 0, sizeof(bucket_chain*)*directorySize_);

			if (directorySize_ != fread(bucketAddr, sizeof(long),
							directorySize_, dataFile_))
				return false;

			for (size_t i=0; i<directorySize_; i++) {
				if (bucketAddr[i] != 0) {
					entry_[i] = new bucket_chain(sfh_.bucketSize, _file_lock);
					entry_[i]->fpos = bucketAddr[i];
				}
			}
			ret = true;
		}
	}
	isOpen_ = true;
	return ret;
}


bool hashdb::close()
{
	if( isOpen_ == false )
		return true;
		
	isOpen_ = false;
	flush();

	delete [] bucketAddr;
	bucketAddr = 0;
	delete [] entry_;
	entry_ = 0;
	if(dataFile_) {
		fclose(dataFile_);
		dataFile_ = 0;
	}
	return true;
}

void hashdb::commit() {
	if( !dataFile_ )
		return;
	do{
		share_rwlock lock(_file_lock, 1);
		sfh_.toFile(dataFile_);
		if (directorySize_ != fwrite(bucketAddr, sizeof(long),
					directorySize_, dataFile_) )
		return;
	}
	while(0);
	if (orderedCommit) {
		if( ! entry_ ) return;
		typedef map<long, bucket_chain*> COMMIT_MAP;
		typedef COMMIT_MAP::iterator CMIT;
		COMMIT_MAP toBeWrited;
		queue<bucket_chain*> qnode;
		for (size_t i=0; i<directorySize_; i++) {
			qnode.push( entry_[i]);
		}
		while (!qnode.empty() ) {
			bucket_chain* popNode = qnode.front();
			if ( popNode && popNode->isLoaded && popNode-> isDirty)
			toBeWrited.insert(make_pair(popNode->fpos, popNode) );
			qnode.pop();
			if (popNode && popNode->next ) {
				qnode.push( popNode->next );
			}
		}

		CMIT it = toBeWrited.begin();
		for (; it != toBeWrited.end(); it++) {
			if ( it->second->write( dataFile_ ) )
			--dirtyPageNum_;
		}
	}
	else {
		for (size_t i=0; i<directorySize_; i++) {
			bucket_chain* sc = entry_[i];
			while (sc) {
				if ( sc->write(dataFile_) ) {
					sc = sc->next;
				} else {
					//sc->display();
					assert(0);
				}
			}
		}
	}
	fflush(dataFile_);
}

void hashdb::flush() {
#ifdef DEBUG
	ClockTimer timer;
#endif
	commit();
#ifdef DEBUG
	printf("commit elapsed 1 ( actually ): %lf seconds\n",
			timer.elapsed() );
#endif
	//ScopedWriteLock<LockType> lock(flushLock_, 1);
	share_rwlock lock(_flush_lock, 1);
	unload_();
}

void hashdb::unload_() {
	if(entry_) {
		for (size_t i=0; i<directorySize_; i++) {
			if (entry_[i]) {
				delete entry_[i];
				entry_[i] = 0;
			}
		}
	}
	activeNum_ = 0;
}

void hashdb::display(std::ostream& os, bool onlyheader ) {
	sfh_.display(os);
	os<<"activeNum: "<<activeNum_<<endl;
	os<<"dirtyPageNum "<<dirtyPageNum_<<endl;

	if ( !onlyheader) {
		for (size_t i=0; i<directorySize_; i++) {
			os<<"["<<i<<"]: ";
			if (entry_[i])
			entry_[i]->display(os);
			os<<endl;
		}
	}
}

double hashdb::loadFactor() {
	int nslot = 0;
	for (size_t i=0; i<directorySize_; i++) {
		bucket_chain* sc = entry_[i];
		while (sc) {
			nslot += sc->num;
			sc = loadNext_(sc);
		}
	}
	if (nslot == 0)
		return 0.0;
	else
		return double(sfh_.numItems)/nslot;
}

void hashdb::setDirty_(bucket_chain* bucket) {
	if( !bucket->isDirty ) {
		++dirtyPageNum_;
		bucket->isDirty = true;
	}
}

bool hashdb::load_(bucket_chain* bucket) {
	if (bucket && !bucket->isLoaded ) {
		++activeNum_;
		return bucket->load(dataFile_);
	}
	return false;
}

bucket_chain* hashdb::allocateBlock_() {
	bucket_chain* newBlock;
	newBlock = new bucket_chain(sfh_.bucketSize, _file_lock);
	newBlock->str = new char[sfh_.bucketSize-sizeof(long)-sizeof(int)];
	memset(newBlock->str, 0, sfh_.bucketSize-sizeof(long)-sizeof(int));
	newBlock->isLoaded = true;
	newBlock->isDirty = true;

	newBlock->fpos = sizeof(hash_header) + sizeof(long)*directorySize_
	+ sfh_.bucketSize*sfh_.nBlock;

	++sfh_.nBlock;
	++activeNum_;
	++dirtyPageNum_;

	return newBlock;
}

bucket_chain* hashdb::loadNext_(bucket_chain* current) {
	bool loaded = false;
	bucket_chain* next = current->loadNext(dataFile_, loaded);
	if (loaded)
	activeNum_++;
	return next;
}

bool hashdb::isEmptyBucket_(uint32_t idx, bucket_chain* &sa) {
	if(idx >= directorySize_)
	{
		sa = NULL;
		return false;
	}

	if( !entry_[idx] )
	{
		if( bucketAddr[idx] != 0 ) {
			entry_[idx] = new bucket_chain(sfh_.bucketSize, _file_lock);
			entry_[idx] ->fpos = bucketAddr[idx];
		}
	}
	sa = entry_[idx];
	load_(sa);
	while(sa && sa->num <= 0) {
		sa = loadNext_(sa);
	}
	return sa == NULL;
}

void hashdb::safeFlushCache_() {
	if (activeNum_> sfh_.cacheSize) {
	//	ScopedWriteLock<LockType> lock(flushLock_);
		share_rwlock lock(_flush_lock, 1);
		flushCacheImpl_();
	}
}

void hashdb::flushCache_() {
	if (activeNum_> sfh_.cacheSize) {
		flushCacheImpl_();
	}

}

void hashdb::safeFlushCache_(Cursor &locn)
{
	if (activeNum_> sfh_.cacheSize) {
		char key[1024];
		size_t ksz = 1024;
		char value[1024*64];
		size_t vsz = 1024*64;
		get(locn, key, ksz, value, vsz);
		{
//			ScopedWriteLock<LockType> lock(flushLock_);
			share_rwlock lock(_flush_lock, 1);
			flushCacheImpl_();
		}
		search(key, ksz, locn);
	}
}

void hashdb::flushCacheImpl_()
{
#ifdef DEBUG
	cout<<"cache is full..."<<endl;
	sfh_.display();
	cout<<activeNum_<<" vs "<<sfh_.cacheSize <<endl;
	cout<<"dirtyPageNum: "<<dirtyPageNum_<<endl;
#endif

	bool commitCondition = dirtyPageNum_ >= (activeNum_ * 0.5);
	if (unloadAll) {
		flush();
#ifdef DEBUG
		cout<<"\n====================================\n"<<endl;
		cout<<"cache is full..."<<endl;
		sfh_.display();
		cout<<activeNum_<<" vs "<<sfh_.cacheSize <<endl;
		cout<<"dirtyPageNum: "<<dirtyPageNum_<<endl;
#endif
		return;
	}
	else {
		if( delayFlush && commitCondition)
			commit();
		for (size_t i=0; i<directorySize_; i++) {
			bucket_chain* sc = entry_[i];
			while (sc) {
				if (sc->isLoaded && !sc->isDirty)
				sh_cache_.insert(make_pair(sc->level, sc));
				sc = sc->next;
			}
		}

		for (CacheIter it = sh_cache_.begin(); it != sh_cache_.end(); it++) {
			if ( it->second->unload() )
			--activeNum_;
		}

		if(delayFlush && commitCondition)
			fflush(dataFile_);
		sh_cache_.clear();
	}
}

template<typename AM>
bool hashdb::dump(AM& other) {
	if (!is_open() )
		open();
	if( !other.is_open() )
	{
		if( !other.open() )
		return false;
	}
	Cursor locn = get_first_locn();
	char* key = new char[sfh_.bucketSize];
	char * value = new char[sfh_.bucketSize];
	size_t kl;
	size_t vl;
	while(get(locn, key, kl, value, vl)) {
		other.insert(key, kl, value, vl);
		if( !seq(locn) )
		break;
	}
	delete [] key;
	delete [] value;
	return true;
}

bool hashdb::dump2f(const string& fileName)
{
	hashdb other(fileName);
	
	other.setPageSize(sfh_.bucketSize);
	other.setDegree(sfh_.dpow);
	other.setCacheSize(sfh_.cacheSize);

	if( !other.open() )
	return false;
	return dump( other );
}

void hashdb::optimize() {
	commit();
	string tempfile = fileName_+ ".swap";
	dump2f(tempfile);
	close();
	std::remove(fileName_.c_str());
	std::rename(tempfile.c_str(), fileName_.c_str() );
	std::remove(tempfile.c_str());
	open();
}
