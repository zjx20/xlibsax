#include "btreedb.h"

using namespace std;
using namespace sax;


// The constructor simply sets up the different data members
btreedb::btreedb(const std::string& fileName , bool thread_safe, cmpfunc *comp){
	_root = 0;
	_isDelaySplit = true;
	_autoTunning = false;
	_optimizeNum = 8192*2;
	_comp = comp;
	if(thread_safe)
	{
		_fileLock = g_share_init();
		_flushLock = g_share_init();
	}
	else
	{
		_fileLock = NULL;
		_flushLock = NULL;
	}

	_fileName = fileName;

	int len = _fileName.size();
	if (_fileName[len-1] == '#') {
		_isDelaySplit = false;
		_fileName.erase(len-1);
	}
	_dataFile = 0;
	_cacheSize = 0;
	_isOpen = false;

	_activeNodeNum = 0;
	_dirtyPageNum = 0;

	_flushCount = 0;

}

btreedb::~btreedb() {
	close();
}

void btreedb::setBtreeMode(bool autoTuning, unsigned int optimizeNum, bool delaySplit)
{
	_isDelaySplit = delaySplit;
	_autoTunning = autoTuning;
	_optimizeNum = optimizeNum;
}

void btreedb::setMaxKeys(size_t maxkeys) {
	assert( _isOpen == false );
	_sfh.maxKeys = maxkeys;
	if(_sfh.maxKeys < 6)
	{
		cout<<"Note: maxKeys at least 6.\nSet to 6 automatically.\n";
		_sfh.maxKeys = 6;
	}
}

void btreedb::setDegree(size_t degree)
{
	assert( _isOpen == false );
	_sfh.maxKeys = 2*degree;
	if(_sfh.maxKeys < 6)
	{
		cout<<"Note: maxKeys at leaset 6.\nSet to 6 automatically.\n";
		_sfh.maxKeys = 6;
	}
}

void btreedb::setPageSize(size_t pageSize) {
	assert( _isOpen == false );
	_sfh.pageSize = pageSize;
}

void btreedb::setCacheSize(size_t sz) {
	_sfh.cacheSize = sz;
	_cacheSize = sz;
}

void btreedb::setFileName(const std::string& fileName ){
	_fileName = fileName;		
}

std::string btreedb::getFileName() const {
	return _fileName;
}

bool btreedb::is_open()
{
	return _isOpen;
}

void btreedb::clear() {
	close();
	std::remove(_fileName.c_str() );
	_sfh.initialize();
	open();
}

bool btreedb::close() {
	if( !_isOpen)
		return true;				
	_isOpen = false;
	flush();
	//note that _root can be  NULL, if there is no items.
	if (_root) {
		delete _root;
		_root = 0;
	}
	if (_dataFile != 0) {
		fflush(_dataFile);
		fclose(_dataFile);
		_dataFile = 0;
	}
	return true;
}

template<typename AM>
bool btreedb::dump(AM& other) {
	if (!is_open() )
	open();
	if( !other.is_open() )
	{
		if( !other.open() )
		return false;
	}
	char *key = new char[_sfh.pageSize];
	char *val = new char[_sfh.pageSize];
	size_t klen, vlen;// = _sfh.pageSize;
	
	Cursor locn = get_first_locn();
	while(get(locn, key, klen, val, vlen)) {
		other.insert(key, klen, val, vlen);
		if( !seq(locn) )
			break;
	}
	delete [] key;
	delete [] val;
	return true;
}

bool btreedb::dump2f(const string& fileName)
{
	btreedb other(fileName,_comp);

	other.setPageSize(_sfh.pageSize);
	other.setMaxKeys(_sfh.maxKeys);
	other.setCacheSize(_sfh.cacheSize);

	if( !other.open() )
	return false;
	return dump(other);
}

void* btreedb::find(const void* key, size_t klen, size_t &vlen) {
	Cursor locn;
	if( search(key, klen, locn) ) {
		share_rwlock lock(_flushLock, 0);
		vlen = ((size_t*)(locn.first->values[locn.second]))[0];
		char *aa = new char[vlen];
		memcpy(aa, locn.first->values[locn.second] + sizeof(size_t), vlen);
		return aa;
	//	return new ValueType(locn.first->values[locn.second]);
	}
	else return NULL;
}

size_t btreedb::find(const void* key, size_t klen, void *val) {
	size_t len;
	const void * vv = find2(key, klen, len);
	if(vv)
	{
		memcpy(val, vv, len);
		return len;
	}
	return (size_t)(-1);
}

bool btreedb::get(const void * key, size_t klen, void* value, size_t &vlen)
{
	_safeFlushCache();
	//ScopedReadLock<LockType> lock(_flushLock);
	share_rwlock lock(_flushLock, 0);
	Cursor locn;
	if( search_(key, klen, locn) ) {
		//cout<<"get "<<endl;
		vlen = ((size_t*)(locn.first->values[locn.second]))[0];
		value = locn.first->values[locn.second];
		memcpy(value, locn.first->values[locn.second] + sizeof(size_t), vlen);
		return true;
	}
	return false;
}

const void* btreedb::find2(const void * key, size_t klen, size_t & vlen) {
	Cursor locn;
	if( search(key, klen, locn) ) {
		share_rwlock lock(_flushLock, 0);
		vlen = ((size_t*)(locn.first->values[locn.second]))[0];
		return locn.first->values[locn.second] + sizeof(size_t);
	}
	else return NULL;
}

int btreedb::num_items() {
	return _sfh.numItems;
}


/**
 *  \brief get the cursor of the first item.
 *
 */
inline btreedb::Cursor btreedb::get_first_locn()
{
	share_rwlock lock(_flushLock, 0);
	btree_node* node = getRoot();
	while( node && !node->isLeaf )
	node = node->loadChild(0, _dataFile);
	return Cursor(node, 0);
}

btreedb::Cursor btreedb::get_last_locn()
{
	share_rwlock lock(_flushLock, 0);
	btree_node* node = getRoot();
	while(node && !node->isLeaf ) {
		node = node->loadChild(node->objCount, _dataFile);
	}
	return Cursor(node, node->objCount-1);
}

inline bool btreedb::seq(Cursor& locn, void* key, size_t klen, void* value, size_t vlen, bool sdir)
{
	bool ret = seq(locn, sdir);
	get(locn, key, klen, value, vlen);
	return ret;
}


inline bool btreedb::seq(Cursor& locn, bool sdir)
{
	_flushCache(locn);
	//ScopedReadLock<LockType> lock(_flushLock);
	share_rwlock lock(_flushLock, 0);
	return seq_(locn, sdir);
}

inline void btreedb::commit() {
	if(_root) {
		_flush(_root, _dataFile);
		_sfh.rootPos = _root->fpos;
	}

	if( !_dataFile )return;

	if ( 0 != fseek(_dataFile, 0, SEEK_SET)) {
		abort();
	}

	//write back the fileHead later, for overflow may occur when
	//flushing.
	_sfh.toFile(_dataFile);
	fflush(_dataFile);
}

void btreedb::display(std::ostream& os, bool onlyheader) {
	_sfh.display(os);
	os<<"activeNum: "<<_activeNodeNum<<endl;
	os<<"dirtyPageNum: "<<_dirtyPageNum<<endl;
	if(!onlyheader && _root)_root->display(os);
}

inline bool btreedb::search(const void* key, size_t klen, Cursor& locn) {
	_safeFlushCache();
	//ScopedReadLock<LockType> lock(_flushLock);
	share_rwlock lock(_flushLock, 0);
	return search_(key, klen, locn);
}

inline btree_node* btreedb::getRoot() {
	if( _root == NULL ) {
		_root = new btree_node(_sfh, _fileLock, _activeNodeNum, _comp);
		_root->fpos = _sfh.rootPos;
		_root->read(_dataFile);
	}
	return _root;
}

void btreedb::optimize() {
	optimize_();
	//commit();
	string tempfile = _fileName+ ".swap";
	dump2f(tempfile);
	close();
	std::remove(_fileName.c_str());
	std::rename(tempfile.c_str(), _fileName.c_str() );
	std::remove(tempfile.c_str());
	open();
}

inline void btreedb::fillCache() {
	queue<btree_node*> qnode;
	qnode.push( getRoot() );

	while ( !qnode.empty() )
	{
		btree_node* popNode = qnode.front();
		qnode.pop();
		if (popNode && !popNode->isLeaf && popNode->isLoaded ) {
			for(size_t i=0; i<=popNode->objCount; i++)
			{
				if( _activeNodeNum> _sfh.cacheSize )
				goto LABEL;
				btree_node* node = popNode->loadChild(i, _dataFile);
				if( node )
				qnode.push( node );

			}
		}
	}
	LABEL: return;
}

inline void btreedb::_setDirty(btree_node* node) {
	if( !node->isDirty ) {
		++_dirtyPageNum;
		node->setDirty(true);
	}
}

inline void btreedb::_flushCache() {
	getRoot();
	++_flushCount;
	if( _activeNodeNum> _sfh.cacheSize )
	{
		_flushCacheImpl();
	}
}

inline void btreedb::_safeFlushCache() {
	getRoot();
	++_flushCount;
	if( _activeNodeNum> _sfh.cacheSize )
	{
		share_rwlock lock(_flushLock, 1);
		_flushCacheImpl();
	}
}

//for seq, reset Cursor
inline void btreedb::_flushCache(Cursor& locn) {
	getRoot();
	if( _activeNodeNum> _sfh.cacheSize )
	{
		char key[_sfh.pageSize];
		char value[_sfh.pageSize];
		size_t sz;
		size_t sz2;
		get(locn, key, sz, value, sz2);
		{
			//cout<<"_flushCache(locn)"<<endl;
			share_rwlock lock(_flushLock, 1);
			_flushCacheImpl();
		}
		search(key, sz, locn);
	}
}

inline void btreedb::_flushCacheImpl(bool quickFlush) {
	if( _activeNodeNum < _sfh.cacheSize )
	return;
#ifdef  DEBUG
	cout<<"\n\ncache is full..."<<endl;
	cout<<"activeNum: "<<_activeNodeNum<<endl;
	cout<<"dirtyPageNum: "<<_dirtyPageNum<<endl;
	//display();
#endif
	ClockTimer timer;
	if( !quickFlush )
	commit();

#ifdef DEBUG
	printf("commit elapsed 1 ( actually ): %lf seconds\n",
			timer.elapsed() );
#endif    
	if(unloadAll) {
		for(size_t i=0; i<_root->objCount+1; i++)
		{
			_root->children[i]->unload();
		}
		if(_activeNodeNum !=1 ) {
			cout<<"Warning! multi-thread enviroment"<<endl;
			_activeNodeNum = 1;
		}
#ifdef DEBUG
		cout<<"\n\nstop unload..."<<endl;
		cout<<_activeNodeNum<<" vs "<<_sfh.cacheSize <<endl;
		cout<<"dirtyPageNum: "<<_dirtyPageNum<<endl;
#endif
		return;
	}
	else {

		queue<btree_node*> qnode;
		qnode.push(_root);

		size_t popNum = 0;
		size_t escapeNum = _activeNodeNum>>1;
		btree_node* interval = NULL;
		while ( !qnode.empty() ) {
			btree_node* popNode = qnode.front();
			qnode.pop();
			popNum++;

			if( popNum >= escapeNum )
			{
				if( popNode == interval )
				break;

				if( interval == NULL && !popNode->isLeaf ) {
					interval = popNode->children[0];
				}

				if( popNode->isDirty && quickFlush)
				_flush(popNode, _dataFile);

				popNode->unload();
			}

			if (popNode && popNode->isLoaded && !popNode->isLeaf) {
				for(size_t i=0; i<popNode->objCount+1; i++)
				{
					if( popNode->children[i] ) {
						qnode.push( popNode->children[i] );
					}
					else
					{
						//cout<<"corrupted nodes!!!"<<endl;
					}

				}
			}

		}

#ifdef DEBUG
		cout<<"stop unload..."<<endl;
		cout<<_activeNodeNum<<" vs "<<_sfh.cacheSize <<endl;
		//display();
#endif
		fflush(_dataFile);
	}

}

inline btree_node* btreedb::_allocateNode() {
	btree_node* newNode;

	newNode = new btree_node(_sfh, _fileLock, _activeNodeNum, _comp);
	newNode->isLoaded = true;
	newNode->isDirty = true;
	//assert(sizeof(CbFileHeader) < 1024);
	newNode->fpos = sizeof(_sfh) + _sfh.pageSize*(_sfh.nPages+_sfh.oPages);

	++_sfh.nPages;
	++_dirtyPageNum;
	++_activeNodeNum;
	
	return newNode;
}

inline btreedb::Cursor btreedb::_findPred(btree_node* node) {
	Cursor ret(NULL, (size_t)-1);
	btree_node* child = node;
	while (!child->isLeaf) {
		child = child->loadChild(child->objCount, _dataFile);
	}
	ret.first = child;
	ret.second = child->objCount - 1;
	return ret;
}

inline btreedb::Cursor btreedb::_findSucc(btree_node* node) {
	Cursor ret(NULL, (size_t)-1);
	btree_node* child = node;
	while (!child->isLeaf) {
		child = child->loadChild(0, _dataFile);
	}
	ret.first = child;
	ret.second = 0;
	return ret;
}

inline void btreedb::optimize_(bool autoTuning) {
	commit();
	double ofactor = double(_sfh.oPages)/double(_sfh.nPages)+1;
	int pfactor = int(ofactor);

	//auto adapt cache size.
	setCacheSize( _sfh.cacheSize/pfactor );

	if( autoTuning ) {

		string tempfile = _fileName + ".swap";
		btreedb other(tempfile);
		if( ofactor> 0.1 )
		{

			double dfactor = ofactor/pfactor;

			other.setPageSize( _sfh.pageSize*(pfactor+1) );
			other.setCacheSize( _sfh.cacheSize/pfactor );
			other.setMaxKeys( int(_sfh.maxKeys/dfactor) );
			other.open();
			dump(other);
			other.close();
			close();
			std::remove(_fileName.c_str() );
			std::rename(tempfile.c_str(), _fileName.c_str() );
			std::remove(tempfile.c_str());
			open();
		}
	}
}


bool btreedb::search_(const void * key, size_t klen, Cursor& locn) {
	//do Flush, when cache is full
	if ( !_isOpen)
		return false;
	//cout<<"before flush"<<endl;
	//_safeFlushCache();

	//cout<<"acquire read lock"<<endl;
	locn.first = 0;
	locn.second = (size_t)-1;

	btree_node* temp = getRoot();
	while (1) {
		if ( !temp)
			return false;
		int i = temp->objCount;
		int low = 0;
		int high = i-1;
		int compVal;
		//upper bound
		while (low <= high) {
			int mid = (low+high)/2;
			size_t tklen = ((size_t*)(temp->keys[mid]))[0];
			compVal = _comp(key, klen, temp->keys[mid] + sizeof(size_t), tklen);
			if (compVal == 0) {
				locn.first = temp;
				locn.second = mid;
				return true;
			} else if (compVal < 0)
				high = mid-1;
			else {
				low = mid+1;
			}
		}

		if (!temp->isLeaf) {
			temp = temp->loadChild(low, _dataFile);
		} else {
			locn.first = temp;
			locn.second = low;

			if (low >= (int)temp->objCount) {
				locn.second = low - 1;
				//_flushLock.release_read_lock();
				seq_(locn);
				return false;
			}
			break;
		}
	}
	//_flushLock.release_read_lock();
	return false;
}

// Splits a child node, creating a new node. The median value from the
// full child is moved into the *non-full* parent. The keys above the
// median are moved from the full child to the new child.
void btreedb::_split(btree_node* parent, size_t childNum, btree_node* child) {

	//display();
	size_t i = 0;
	size_t leftCount = (child->objCount)>>1;
	size_t rightCount = child->objCount-1-leftCount;
	btree_node* newChild = _allocateNode();
	newChild->childNo = childNum+1;

	newChild->isLeaf = child->isLeaf;
	newChild->setCount(rightCount);

	// Put the high values in the new child, then shrink the existing child.
	for (i = 0; i < rightCount; i++) {
		newChild->keys[i] = child->keys[leftCount +1 + i];
		newChild->values[i] = child->values[leftCount +1 + i];
		
		child->keys[leftCount +1 + i] = 0;
		child->values[leftCount +1 + i] = 0;
	}

	if (!child->isLeaf) {
		for (i = 0; i < rightCount+1; i++) {
			btree_node* mover = child->children[leftCount +1 + i];
			newChild->children[i] = mover;
			mover->childNo = i;
			mover->parent = newChild;
			child->children[leftCount +1 + i] = 0;
		}
	}

	char* savekey = child->keys[leftCount];
	char* savevalue = child->values[leftCount];
	child->setCount(leftCount);
	
	child->keys[leftCount] = 0;
	child->values[leftCount] = 0;

	// Move the child pointers above childNum up in the parent
	parent->setCount(parent->objCount + 1);
	for (i = parent->objCount; i> childNum + 1; i--) {
		parent->children[i] = parent->children[i - 1];
		//if(parent->children[i])
		parent->children[i]->childNo = i;
	}
	parent->children[childNum + 1] = newChild;
	newChild->childNo = childNum + 1;
	newChild->parent = parent;

	for (i = parent->objCount - 1; i> childNum; i--) {
		parent->keys[i] = parent->keys[i - 1];
		parent->values[i] = parent->values[i - 1];
	}
	parent->keys[childNum] = savekey;
	parent->values[childNum] = savevalue;
	_setDirty(child);
	_setDirty(newChild);
	_setDirty(parent);
}

//split two full leaf nodes into tree 2/3 ful nodes.
void btreedb::_split3Leaf(btree_node* parent, size_t childNum) {

	size_t i = 0;
	size_t count1 = (_sfh.maxKeys<<1)/3;
	size_t count2 = _sfh.maxKeys - _sfh.maxKeys/3-1;
	size_t count3 = (_sfh.maxKeys<<1) -1 -count1 -count2;

	btree_node* child1 = parent->loadChild(childNum, _dataFile);
	btree_node* child2 = parent->loadChild(childNum+1, _dataFile);

	btree_node* newChild = _allocateNode();

	//swap newchild's fpos with child2	
	long tempfpos = child2->fpos;
	child2->fpos = newChild->fpos;
	newChild->fpos = tempfpos;

	newChild->isLeaf =true;
	newChild->setCount(count3);
	newChild->parent = parent;

	char* tkey1 = child1->keys[(_sfh.maxKeys<<1)/3];
	char* tvalue1 = child1->values[(_sfh.maxKeys<<1)/3];
	char* tkey2 = child2->keys[_sfh.maxKeys/3];
	char* tvalue2 = child2->values[_sfh.maxKeys/3];
	
	
	// Put the high values in the new child, then shrink the existing child.
	for (i = 0; i < _sfh.maxKeys - count1 -1; i++) {
		newChild->keys[i] = child1->keys[count1+1+i];
		newChild->values[i] = child1->values[count1+1+i];
		
		child1->keys[count1+1+i] = 0;
		child1->values[count1+1+i] = 0;
	}

	newChild->keys[ _sfh.maxKeys - count1 - 1] = parent->keys[childNum];
	newChild->values[ _sfh.maxKeys - count1 - 1] = parent->values[childNum];
	
	for (i = _sfh.maxKeys - count1; i < count3; i++) {
		newChild->keys[i] = child2->keys[i-_sfh.maxKeys+count1];
		newChild->values[i] = child2->values[i-_sfh.maxKeys+count1];
	}

	for (i=0; i<count2; i++) {
		child2->keys[i] = child2->keys[_sfh.maxKeys/3+1+i];
		child2->values[i] = child2->values[_sfh.maxKeys/3+1+i];
		
		child2->keys[_sfh.maxKeys/3+1+i] = 0;
		child2->values[_sfh.maxKeys/3+1+i] = 0;
	}

	child1->setCount(count1);
	child2->setCount(count2);
	
	child1->keys[count1] = 0;
	child1->values[count1] = 0;
	child2->keys[count2] = 0;
	child2->values[count2] = 0;

	// Move the child pointers above childNum up in the parent
	parent->setCount(parent->objCount + 1);
	parent->keys[childNum] = tkey1;
	parent->values[childNum] = tvalue1;

	for (i = parent->objCount; i> childNum + 2; i--) {
		parent->children[i] = parent->children[i - 1];
		parent->children[i]->childNo = i;
	}
	parent->children[childNum + 1] = newChild;
	newChild->childNo = childNum + 1;

	for (i = parent->objCount-1; i> childNum+1; i--) {
		parent->keys[i] = parent->keys[i - 1];
		parent->values[i] = parent->values[i - 1];
	}
	parent->keys[childNum+1] = tkey2;
	parent->values[childNum+1] = tvalue2;
	parent->children[childNum+2] = child2;
	child2->childNo = childNum+2;

	_setDirty(child1);
	_setDirty(child2);
	_setDirty(newChild);
	_setDirty(parent);
}

btree_node* btreedb::_merge(btree_node * parent, size_t objNo) {
	size_t i = 0;
	btree_node* c1 = parent->loadChild(objNo, _dataFile);
	btree_node* c2 = parent->loadChild(objNo+1, _dataFile);
	size_t _minDegree = _sfh.maxKeys/3;

	for (i = 0; i < _minDegree - 1; i++) {
		c1->keys[_minDegree+i] = c2->keys[i];
		c1->values[_minDegree+i] = c2->values[i];
		c2->keys[i] = 0;
		c2->values[i] = 0;

	}
	if (!c2->isLeaf) {
		for (i = 0; i < _minDegree; i++) {
			size_t newPos = _minDegree + i; 
			c2->loadChild(i, _dataFile);
			c1->children[newPos] = c2->children[i];
			c1->children[newPos]->childNo = newPos;
			c1->children[newPos]->parent = c1;//wps add it!
			c2->children[i] = 0;
		}
	}

	// Put the parent into the middle
	c1->keys[_minDegree-1] = parent->keys[objNo];
	c1->values[_minDegree-1] = parent->values[objNo];
	c1->setCount(2*_minDegree-1);

	// Reshuffle the parent (it has one less object/child)
	for (i = objNo + 1; i < parent->objCount; i++) {
		parent->keys[i-1] = parent->keys[i];
		parent->values[i-1] = parent->values[i];
		parent->loadChild(i+1, _dataFile);
		parent->children[i] = parent->children[i + 1];
		parent->children[i]->childNo = i;
	}
	parent->setCount(parent->objCount-1);
	
	parent->keys[parent->objCount] = 0;
	parent->values[parent->objCount] = 0;
	parent->children[parent->objCount + 1] = 0;

	if (parent->objCount == 0) {
		parent = c1;
	}

	// Note that c2 will be release. The node will be deallocated
	// and the node's location on
	// disk will become inaccessible.
	c2->objCount = 0;
	delete c2;
	c2 = 0;

	_setDirty(c1);
	_setDirty(parent);

	// Return a pointer to the new child.
	return c1;
}

bool btreedb::insert(const void* key, size_t klen, const void* value, size_t vlen) {
	if ( !_isOpen)
		return false;
	if (_sfh.numItems == _optimizeNum)
		optimize_(_autoTunning);

	_flushCache();
	getRoot();
	if (_root->objCount >= _sfh.maxKeys) {
		// Growing the tree happens by creating a new
		// node as the new root, and splitting the
		// old root into a pair of children.
		btree_node* oldRoot = _root;

		_root = _allocateNode();
		_root->setCount(0);
		_root->isLeaf = false;
		_root->children[0] = oldRoot;
		oldRoot->childNo = 0;
		oldRoot->parent = _root;
		_split(_root, 0, oldRoot);
		goto L0;
	} else {
		L0: register btree_node* node = _root;

		char * tkey = new char[klen + sizeof(size_t)];
		char * tval = new char[vlen + sizeof(size_t)];
		
		char *ptr = tkey;
		char *ptr1 = tval;
		memcpy(ptr, &klen, sizeof(size_t));
		ptr += sizeof(size_t);
		memcpy(ptr, key, klen);
		
		memcpy(ptr1, &vlen, sizeof(size_t));
		ptr1 += sizeof(size_t);
		memcpy(ptr1, value, vlen);

		L1: register size_t i = node->objCount;
		register int low = 0;
		register int high = i-1;
		register int mid;
		register int compVal;
		while (low<=high) {
			mid = (low+high)>>1;
			size_t mlen = ((size_t*)(node->keys[mid]))[0];
			compVal = _comp(key, klen,  node->keys[mid] + sizeof(size_t), mlen);
			if (compVal == 0)
			{
				delete [] tkey;
				delete [] tval;
				return false;
			}
			else if (compVal < 0)
				high = mid-1;
			else {
				low = mid+1;
			}
		}

		// If the node is a leaf, we just find the location to insert
		// the new item, and shuffle everything else up.
		if (node->isLeaf) {
			node->setCount(node->objCount + 1);
			for (; (int)i> low; i--) {
				node->keys[i] = node->keys[i-1];
				node->values[i] = node->values[i-1];
			}
			
			node->keys[low] = tkey;
			node->values[low] = tval;
			_setDirty(node);
			
			++_sfh.numItems;
			return true;
		}

		// If the node is an internal node, we need to find
		// the location to insert the value ...
		else {
			// Load the child into which the value will be inserted.
			btree_node* child = node->loadChild(low, _dataFile);

			//If the child node is full , we will insert into its adjacent nodes, and if bothe are
			//are full, we will split the two node to three nodes.
			if (child->objCount >= _sfh.maxKeys) {
				if ( !child->isLeaf || !_isDelaySplit) {
					_split(node, low, child);
					size_t sz = ((size_t*)(node->keys[low]))[0];
					compVal = _comp(key, klen, node->keys[low] + sizeof(size_t), sz);
					if (compVal == 0)
					{
						delete [] tkey;
						delete [] tval;
						return false;
					}
					if (compVal> 0) {
						++low;
					}
					child = node->loadChild(low, _dataFile);
				} else {
					btree_node* adjNode;
					int splitNum = low;
					if ((size_t)low < node->objCount) {
						adjNode = node->loadChild(low+1, _dataFile);
						if (adjNode->objCount < _sfh.maxKeys) {

							//case: child's last key equal inserting key
							size_t sz = ((size_t*)(child->keys[child->objCount-1]))[0];
							if (_comp(child->keys[child->objCount-1] + sizeof(size_t), sz, key, klen)==0) {
								delete [] tkey;
								delete [] tval;
								return false;
							}
							adjNode->setCount(adjNode->objCount+1);
							for (size_t j=child->objCount-1; j>0; j--) {
								adjNode->keys[j] = adjNode->keys[j-1];
								adjNode->values[j] = adjNode->values[j-1];
							}
							adjNode->keys[0] = node->keys[low];
							adjNode->values[0] = node->values[low];
							_setDirty(adjNode);
							_setDirty(node);

							//case: all of the keys in child are less than inserting keys.
							sz = ((size_t*)(child->keys[child->objCount-1]))[0];
							if (_comp(child->keys[child->objCount-1] + sizeof(size_t), sz, key, klen)<0) {
								node->keys[low] = tkey;
								node->values[low] = tval;
								++_sfh.numItems;
								return true;
							}

							//case: insert the item into the new child.
							node->keys[low] = child->keys[child->objCount-1];
							node->values[low] = child->values[child->objCount-1];
							child->keys[child->objCount-1] = 0;
							child->values[child->objCount-1] = 0;
							//child->setDirty(true);
							_setDirty(child);
							child->setCount(child->objCount-1);
							node = child;

							goto L1;
						}
					}

					//case: the right sibling is full or no right sibling
					if (low>0) {
						adjNode = node->loadChild(low-1, _dataFile);
						//case: left sibling is no full
						if (adjNode->objCount < _sfh.maxKeys) {
							//cacheL child's first key equal inserting key,do nothing
							size_t sz = ((size_t *)(child->keys[0]))[0];
							int _ans = _comp(child->keys[0] + sizeof(size_t), sz, key, klen);
							if (_ans == 0) {
								delete [] tkey;
								delete [] tval;
								return false;
							}

							adjNode->setCount(adjNode->objCount+1);
							adjNode->keys[adjNode->objCount-1]
									= node->keys[low-1];
							adjNode->values[adjNode->objCount-1]
									= node->values[low-1];
							_setDirty(adjNode);
							_setDirty(node);
							//adjNode->setDirty(true);
							//node->setDirty(true);
							//case: all of the keys in child are bigger than inserting keys.
							if (_ans > 0) {
								node->keys[low-1] = tkey;
								node->values[low-1] = tval;
								++_sfh.numItems;
								return true;
							}
							node->keys[low-1] = child->keys[0];
							node->values[low-1] = child->values[0];

							//insert key into child, child's firt item is already put to its parent
							size_t j=1;
							bool ret=true;
							
							//child->setDirty(true);
							_setDirty(child);
							
							//not verygood~ loop
							for (; j < child->objCount; j++) {
								//inserting key exists, mark it
								sz = ((size_t *)(child->keys[j]))[0];
								_ans = _comp(child->keys[j] + sizeof(size_t), sz, key, klen);
								if (_ans == 0) {
									ret = false;
								}
								//have found the right place for inserting key.
								if ( ret && _ans > 0 ) {
									child->keys[j-1] = tkey;
									child->values[j-1] = tval;
									++_sfh.numItems;
									return true;
								}
								child->keys[j-1] = child->keys[j];
								child->values[j-1] = child->values[j];
							}
							//inserting key exists
							if ( !ret) {
								child->setCount(child->objCount-1);
								child->keys[child->objCount] = 0;
								child->values[child->objCount] = 0;
								delete [] tkey;
								delete [] tval;
								return false;
							}
							//insert the key at last positon of child
							child->keys[j-1] = tkey;
							child->values[j-1] = tval;
							++_sfh.numItems;
							return true;
						}
					}

					//case: both nodes are full
					if ( (size_t)splitNum>0) {
						splitNum = splitNum -1;
					}
					_split3Leaf(node, splitNum);
					
					size_t sz1 = ((size_t *)(node->keys[splitNum]))[0];
					size_t sz2 = ((size_t *)(node->keys[splitNum+1]))[0];
					int ans1 = _comp(node->keys[splitNum] + sizeof(size_t), sz1, key, klen);
					if (ans1 == 0)
					{
						delete [] tkey;
						delete [] tval;
						return false;
					}
					int ans2 = _comp(node->keys[splitNum+1] + sizeof(size_t), sz2, key, klen);
					if (ans2 == 0)
					{
						delete [] tkey;
						delete [] tval;
						return false;
					}
					
					if (ans1 > 0) {
						child = node->loadChild(splitNum, _dataFile);
					} else if (ans2 > 0) {
						child=node->loadChild(splitNum+1, _dataFile);
					} else {
						child=node->loadChild(splitNum+2, _dataFile);
					}
				}
			}
			// Insert the key (recursively) into the non-full child
			// node.
			node = child;
			goto L1;
		}
	}
}

// Write all nodes in the tree to the file given.
void btreedb::_flush(btree_node* node, FILE* f) {

	// Bug out if the file is not valid
	if (!f) {
		return;
	}

	if (orderedCommit) {
		typedef map<long, btree_node*> COMMIT_MAP;
		typedef COMMIT_MAP::iterator CMIT;
		COMMIT_MAP toBeWrited;
		queue<btree_node*> qnode;
		qnode.push(node);
		while (!qnode.empty() ) {
			btree_node* popNode = qnode.front();
			if (popNode->isLoaded && popNode-> isDirty)
				toBeWrited.insert(make_pair(popNode->fpos, popNode) );
			qnode.pop();
			if (popNode && !popNode->isLeaf) {
				for (size_t i=0; i<popNode->objCount+1; i++) {
					if (popNode->children && popNode->children[i])
						qnode.push(popNode->children[i]);
				}
			}
		}

		CMIT it = toBeWrited.begin();
		for (; it != toBeWrited.end(); it++) {
			if (it->second->write(f) )
				--_dirtyPageNum;
		}

	} else {

		queue<btree_node*> qnode;
		qnode.push(node);
		while (!qnode.empty()) {
			btree_node* popNode = qnode.front();
			if (popNode && popNode->isLoaded) {
				if (popNode->write(f) )
					--_dirtyPageNum;
			}
			qnode.pop();
			if (popNode && !popNode->isLeaf) {
				for (size_t i=0; i<popNode->objCount+1; i++) {
					if (popNode->children[i])
						qnode.push(popNode->children[i]);
				}
			}
		}

	}

}

// Internal delete function, used once we've identified the
// location of the node from which a key is to be deleted.
bool btreedb::_delete(btree_node* nd, const void* k, size_t klen) {
	if ( !_isOpen)
		return false;
	bool ret = false;
	// Find the object position. op will have the position
	// of the object in op.first, and a flag (op.second)
	// saying whether the object at op.first is an exact
	// match (true) or if the object is in a child of the
	// current node (false). If op.first is -1, the object
	// is neither in this node, or a child node.

	btree_node* node = nd;
	size_t _minDegree = _sfh.maxKeys/3;
	const void *key = k;
	L0: KEYPOS op = node->findPos(key, klen);
	if (op.first != (size_t)-1) // it's in there somewhere ...
	{
		if (op.second == CCP_INTHIS) // we've got an exact match
		{
			// Case 1: deletion from leaf node.
			if (node->isLeaf) {
				//assert(node->objCount >= _minDegree-1);
				node->delFromLeaf(op.first);

				//now node is dirty
				_setDirty(node);
				//_dirtyPages.push_back(node);
				ret = true;
			}
			// Case 2: Exact match on internal leaf.
			else {
				// Case 2a: prior child has enough elements to pull one out.
				node->loadChild(op.first, _dataFile);
				node->loadChild(op.first+1, _dataFile);
				if (node->children[op.first]->objCount >= _minDegree) {
					btree_node* childNode = node->loadChild(op.first, _dataFile);
					Cursor locn = _findPred(childNode);
					
					size_t kl = ((size_t *)(locn.first->keys[locn.second]))[0] + sizeof(size_t);
					size_t vl = ((size_t *)(locn.first->values[locn.second]))[0] + sizeof(size_t);
					char *tk = new char[kl];
					char *tv = new char[vl];
					memcpy(tk, locn.first->keys[locn.second], kl);
					memcpy(tv, locn.first->values[locn.second], vl);
					
					delete [] node->keys[op.first];
					node->keys[op.first] = tk;
					delete [] node->values[op.first];
					node->values[op.first] = tv;
					
					//sdb
					//node->keys[op.first] = locn.first->keys[locn.second];
					//node->values[op.first] = locn.first->values[locn.second];

					//now node is dirty
					_setDirty(node);

					node = childNode;
					key = tk + sizeof(size_t);
					klen = kl - sizeof(size_t);
					goto L0;
				}

				// Case 2b: successor child has enough elements to pull one out.
				else if (node->children[op.first + 1]->objCount >= _minDegree) {
					btree_node* childNode = node->loadChild(op.first + 1,
							_dataFile);
					Cursor locn = _findSucc(childNode);

					size_t kl = ((size_t *)(locn.first->keys[locn.second]))[0] + sizeof(size_t);
					size_t vl = ((size_t *)(locn.first->values[locn.second]))[0] + sizeof(size_t);
					char *tk = new char[kl];
					char *tv = new char[vl];
					memcpy(tk, locn.first->keys[locn.second], kl);
					memcpy(tv, locn.first->values[locn.second], vl);
					
					delete [] node->keys[op.first];
					node->keys[op.first] = tk;
					delete [] node->values[op.first];
					node->values[op.first] = tv;

					//sdb
					//node->keys[op.first] = locn.first->keys[locn.second];
					//node->values[op.first] = locn.first->values[locn.second];

					//now node is dirty
					_setDirty(node);

					node = childNode;
					key = tk + sizeof(size_t);
					klen = kl - sizeof(size_t);

					goto L0;
					//ret = _delete(childNode, dat.get_key());
				}

				// Case 2c: both children have only t-1 elements.
				// Merge the two children, putting the key into the
				// new child. Then delete from the new child.
				else {
					assert(node->children[op.first]->objCount == _minDegree-1);
					assert(node->children[op.first+1]->objCount == _minDegree-1);
					btree_node* mergedChild = _merge(node, op.first);
					node = mergedChild;
					goto L0;
					//ret = _delete(mergedChild, key);
				}
			}
		}

		// Case 3: key is not in the internal node being examined,
		// but is in one of the children.
		else if (op.second == CCP_INLEFT || op.second == CCP_INRIGHT) {
			// Find out if the child tree containing the key
			// has enough elements. If so, we just recurse into
			// that child.

			node->loadChild(op.first, _dataFile);
			if (op.first+1<=node->objCount)
				node->loadChild(op.first+1, _dataFile);
			size_t keyChildPos = (op.second == CCP_INLEFT) ? op.first
					: op.first + 1;
			btree_node* childNode = node->loadChild(keyChildPos, _dataFile);
			if (childNode->objCount >= _minDegree) {
				node = childNode;
				goto L0;
				//ret = _delete(childNode, key);
			} else {
				// Find out if the childNode has an immediate
				// sibling with _minDegree keys.
				btree_node* leftSib = 0;
				btree_node* rightSib = 0;
				size_t leftCount = 0;
				size_t rightCount = 0;
				if (keyChildPos> 0) {
					leftSib = node->loadChild(keyChildPos - 1, _dataFile);
					leftCount = leftSib->objCount;
				}
				if (keyChildPos < node->objCount) {
					rightSib = node->loadChild(keyChildPos + 1, _dataFile);
					rightCount = rightSib->objCount;
				}

				// Case 3a: There is a sibling with _minDegree or more keys.
				if (leftCount >= _minDegree || rightCount >= _minDegree) {
					// Part of this process is making sure that the
					// child node has minDegree elements.

					childNode->setCount(_minDegree);

					// Bringing the new key from the left sibling
					if (leftCount >= _minDegree) {
						// Shuffle the keys and elements up
						size_t i = _minDegree - 1;
						for (; i> 0; i--) {	
							childNode->keys[i] = childNode->keys[i - 1];
							childNode->values[i] = childNode->values[i - 1];
							childNode->children[i + 1] = childNode->children[i];
							if (childNode->children[i + 1]) {
								childNode->children[i + 1]->childNo = i + 1;
							}
						}
						childNode->children[i + 1] = childNode->children[i];
						if (childNode->children[i + 1]) {
							childNode->children[i + 1]->childNo = i +1;
						}

						// Put the key from the parent into the empty space,
						// pull the replacement key from the sibling, and
						// move the appropriate child from the sibling to
						// the target child.

						childNode->keys[0] = node->keys[keyChildPos - 1];
						childNode->values[0] = node->values[keyChildPos - 1];

						node->keys[keyChildPos - 1]
								= leftSib->keys[leftSib->objCount - 1];
						node->values[keyChildPos - 1]
								= leftSib->values[leftSib->objCount - 1];
						
						leftSib->keys[leftSib->objCount - 1] = 0;
						leftSib->values[leftSib->objCount - 1] = 0;
						
						if (!leftSib->isLeaf) {
							childNode->children[0]
									= leftSib->children[leftSib->objCount];
							childNode->children[0]->childNo = 0;
							childNode->children[0]->parent = childNode;
							
							leftSib->children[leftSib->objCount] = 0;
						}
						leftSib->setCount(leftSib->objCount-1);
						//--leftSib->objCount;
						assert(leftSib->objCount >= _minDegree-1);

						//now node is dirty
						_setDirty(leftSib);
						_setDirty(node);
					}

					// Bringing a new key in from the right sibling
					else {
						// Put the key from the parent into the child,
						// put the key from the sibling into the parent,
						// and move the appropriate child from the
						// sibling to the target child node.
						childNode->keys[childNode->objCount - 1]
								= node->keys[op.first];
						childNode->values[childNode->objCount - 1]
								= node->values[op.first];
						node->keys[op.first] = rightSib->keys[0];
						node->values[op.first] = rightSib->values[0];

						if (!rightSib->isLeaf) {
							childNode->children[childNode->objCount]
									= rightSib->children[0];

							childNode->children[childNode->objCount]->childNo
									= childNode->objCount;//wps add it!
							childNode->children[childNode->objCount]->parent
									= childNode;//wps add it!
						}

						// Now clean up the right node, shuffling keys
						// and elements to the left and resizing.
						size_t i = 0;
						for (; i < rightSib->objCount - 1; i++) {
							rightSib->keys[i] = rightSib->keys[i + 1];
							rightSib->values[i] = rightSib->values[i + 1];
							if (!rightSib->isLeaf) {
								rightSib->children[i]
										= rightSib->children[i + 1];

								rightSib->children[i]->childNo = i;
							}
						}
						
						rightSib->keys[i] = 0;
						rightSib->values[i] = 0;
						
						if (!rightSib->isLeaf) {
							rightSib->children[i] = rightSib->children[i + 1];
							rightSib->children[i]->childNo = i;
							rightSib->children[i + 1] = 0;
						}
						rightSib->setCount(rightSib->objCount - 1);
						assert(rightSib->objCount >= _minDegree-1);

						//now node is dirty
						_setDirty(rightSib);
						_setDirty(node);
					}
					node = childNode;

					_setDirty(node);
					goto L0;
				}

				// Case 3b: All siblings have _minDegree - 1 keys
				else {
					assert(node->children[op.first]->objCount == _minDegree-1);
					assert(node->children[op.first+1]->objCount == _minDegree-1);
					btree_node* mergedChild = _merge(node, op.first);
					node = mergedChild;
					goto L0;
				}
			}
		}
	}
	return ret;
}

// Opening the database means that we check the file
// and see if it exists. If it doesn't exist, start a database
// from scratch. If it does exist, load the root node into
// memory.
bool btreedb::open() {

	if (_isOpen)
		return true;

	// We're creating if the file doesn't exist.

	struct stat statbuf;
	bool creating = stat(_fileName.c_str(), &statbuf);

	_dataFile = fopen(_fileName.c_str(), creating ? "w+b" : "r+b");
	if (0 == _dataFile) {
#ifdef DEBUG
		cout <<"SDB Error: open file failed, check if dat directory exists"
		<<endl;
#endif
		return false;
	}

	// Create a new node
	bool ret = false;
	if (creating) {

#ifdef DEBUG
		cout<<"creating btreedb: "<<_fileName<<"...\n"<<endl;
		_sfh.display();
#endif

		_sfh.toFile(_dataFile);

		// If creating, allocate a node instead of
		// reading one.
		_root = _allocateNode();
		_root->isLeaf = true;
		_root->isLoaded = true;
		_root->fpos = _sfh.rootPos;

		commit();
		ret = true;

	} else {

		// when not creating, read the root node from the disk.
		memset(&_sfh, 0, sizeof(_sfh));
		_sfh.fromFile(_dataFile);
		if (_sfh.magic != 0x1a9999a1) {
			cout<<"Error, read wrong file header\n"<<endl;
			assert(false);
			return false;
		}
		if (_cacheSize != 0) {
			//reset cacheSize that has been set before open.
			//cacheSize is dynamic at runtime
			_sfh.cacheSize = _cacheSize;
		}
#ifdef DEBUG
		cout<<"open btreedb: "<<_fileName<<"...\n"<<endl;
		_sfh.display();
#endif

		_root = getRoot();
		ret = true;

	}
	_isOpen = true;
	return ret;
}

// This is the external delete function.
bool btreedb::del(const void* key, size_t klen) {

	if ( !_isOpen)
		return false;
	_flushCache();
	getRoot();
	// Determine if the root node is empty.
	bool ret = (_root->objCount != 0);

	// If we successfully deleted the key, and there
	// is nothing left in the root node and the root
	// node is not a leaf, we need to shrink the tree
	// by making the root's child (there should only
	// be one) the new root. Write the location of
	// the new root to the start of the file so we
	// know where to look
	if (_root->objCount == 0 && !_root->isLeaf) {
		btree_node* node = _root;
		_root = node->children[0];
		_root->parent = 0;
		node->children[0] = 0;
		//node->unloadself();
		delete node;
		node = 0;
	}

	if (_root->objCount == (size_t) -1) {
		return 0;
	}

	// If our root is not empty, call the internal
	// delete method on it.
	ret = _delete(_root, key, klen);

	if (ret)
		--_sfh.numItems;
	return ret;
}

// This method retrieves a record from the database
// given its location.
bool btreedb::get(const Cursor& locn, void* key, size_t &klen, void * value, size_t &vlen) {
	//ScopedReadLock<LockType> lock(_flushLock);
	share_rwlock lock(_flushLock, 0);
	if ((btree_node*)locn.first == 0 || locn.second == (size_t)-1 || locn.second
			>= locn.first->objCount) {
		return false;
	}
	klen = ((size_t*)(locn.first->keys[locn.second]))[0];
	memcpy(key, locn.first->keys[locn.second] + sizeof(size_t), klen);
	
	vlen = ((size_t*)(locn.first->values[locn.second]))[0];
	memcpy(value, locn.first->values[locn.second] + sizeof(size_t), vlen);
	return true;
}

bool btreedb::update(const void* key, size_t klen, const void* value, size_t vlen) {
	if ( !_isOpen)
		return false;
	Cursor locn(NULL, (size_t)-1);
	if (search(key, klen, locn) ) {
		char *cc = new char[vlen + sizeof(size_t)];
		memcpy(cc, &vlen, sizeof(size_t));
		memcpy(cc + sizeof(size_t), value, vlen);
		delete [] locn.first->values[locn.second];
		locn.first->values[locn.second] = cc;
		_setDirty(locn.first);
		return true;
	} else {
		return insert(key, klen, value, vlen);
	}
	return false;
}

// This method finds the record following the one at the
// location given as locn, and copies the record into rec.
// The direction can be either forward or backward.
bool btreedb::seq_(Cursor& locn, bool sdir) {
	if ( !_isOpen)
		return false;
	if (_sfh.numItems <=0) {
		return false;
	}
	getRoot();
	_root->parent = 0;
	//cout<<"seq _flushCache()"<<endl;
	
	if(sdir) return _seqNext(locn);
	
	return _seqPrev(locn);

}

bool btreedb::packData(btree_node *node, size_t it, btreedb::DataPack &p)
{
//	p.clear();
	p.klen = ((size_t*)(node->keys[it]))[0];
	//p.key = new char[p.klen];
	//memcpy(p.key, node->keys[it] + sizeof(size_t), p.klen);
	p.vlen = ((size_t*)(node->values[it]))[0];
	//p.val = new char[p.vlen];
	//memcpy(p.val, node->values[it] + sizeof(size_t), p.vlen);
	p.key = node->keys[it] + sizeof(size_t);
	p.val = node->values[it] + sizeof(size_t);
	return true;
}

bool btreedb::range(size_t start, size_t end, std::vector<btreedb::DataPack> & pp)
{
	if(end < start)
		return false;
	
	btree_node* root = getRoot();
	root->parent = 0;
	btree_node* node = root;
	
	while( node && !node->isLeaf )
		node = node->loadChild(0, _dataFile);
	
	pp.clear();
	size_t iter = 0;
	bool get = false;
	size_t ci = 0;
	size_t count = end - start + 1;
	
	do
	{
		if(node->isLeaf)
		{
			size_t ii = 0;
			get = true;
			if(iter > start) ii = 0;
			else if(iter + node->objCount > start) 
				ii = start - iter;
			else get = false;
			
			//printf("%d\n", ii);
			
			if(get)
			{
				for( ; ii < node->objCount; ++ii)
				{
					DataPack k;
					packData(node, ii, k);
					pp.push_back(k);
					if(pp.size() == count)
						return true; 
				}
			}
			iter += node->objCount;
			
			do{
				ci = node->childNo;
				node = node->parent;
			}
			while(node && node != root && ci == node->objCount);
		}
		else
		{
			if(ci == node->objCount) //root
				break;
			iter++;
			if(iter > start)
			{
				DataPack k;
				packData(node, ci, k);
				pp.push_back(k);
				if(pp.size() == count)
					return true; 
			}
			node = node->loadChild(ci+1, _dataFile);

			while( node && !node->isLeaf )
				node = node->loadChild(0, _dataFile);
		}
	}
	while(node);
	
	return false;
}


// Find the next item in the database given a location. Return
// the subsequent item in rec.
bool btreedb::_seqNext(Cursor& locn) {
	// Set up a couple of convenience values

	bool ret = false;

	btree_node* node = locn.first;
	size_t lastPos = locn.second;
	bool goUp = false; // indicates whether or not we've exhausted a node.

	// If we are starting at the beginning, initialise
	// the locn reference and return with the value set.
	// This means we have to plunge into the depths of the
	// tree to find the first leaf node.
	if ((btree_node*)node == 0) {
		node = _root;
		while ((btree_node*)node != 0 && !node->isLeaf) {
			node = node->loadChild(0, _dataFile);
		}
		if ((btree_node*)node == 0) {
			return false;
		}

		// pre-fetch all nested keys
		btree_node* parent=node->parent;
		for (unsigned int i=1; i<=parent->objCount; i++)
			parent->loadChild(i, _dataFile);

		locn.first = node;
		locn.second = 0;
		return true;
	}

	// Advance the locn object to the next item

	// If we have a leaf node, we don't need to worry about
	// traversing into children ... only need to worry about
	// going back up the tree.
	if (node->isLeaf) {
		// didn't visit the last node last time.
		if (lastPos < node->objCount - 1) {
			locn.second = lastPos + 1;
			return true;
		}
		goUp = (lastPos == node->objCount - 1);
	}

	// Not a leaf, therefore need to worry about traversing
	// into child nodes.
	else {
		node = node->loadChild(lastPos + 1, _dataFile);//_cacheInsert(node);
		while ((btree_node*)node != 0 && !node->isLeaf) {
			node = node->loadChild(0, _dataFile);
		}
		if ((btree_node*)node == 0) {
			return false;
		}

		// pre-fetch all nested keys
		btree_node* parent=node->parent;
		if (parent)
			for (unsigned int i=1; i<=parent->objCount; i++)
				parent->loadChild(i, _dataFile);

		locn.first = node;
		locn.second = 0;
		return true;
	}

	// Finished off a leaf, therefore need to go up to
	// a parent.
	if (goUp) {
		size_t childNo = node->childNo;
		node = node->parent;
		while ((btree_node*)node != 0 && childNo >= node->objCount) {
			childNo = node->childNo;
			node = node->parent;
		}
		if ((btree_node*)node != 0) {
			locn.first = node;
			locn.second = childNo;
			return true;
		}
	}
	//reach the last locn
	++locn.second;
	return ret;
}

// Find the previous item in the database given a location. Return
// the item in rec.
bool btreedb::_seqPrev(Cursor& locn) {
	// Set up a couple of convenience values
	bool ret = false;
	btree_node* node = locn.first;
	size_t lastPos = locn.second;
	bool goUp = false; // indicates whether or not we've exhausted a node.


	// If we are starting at the end, initialise
	// the locn reference and return with the value set.
	// This means we have to plunge into the depths of the
	// tree to find the first leaf node.

	if ((btree_node*)node == 0) {
		node = _root;
		while ((btree_node*)node != 0 && !node->isLeaf) {
			node = node->loadChild(node->objCount, _dataFile);
		}
		if ((btree_node*)node == 0) {
			return false;
		}
		locn.first = node;
		locn.second = node->objCount - 1;

		if (locn.second == size_t(-1))
			return false;
		return true;
	}

	// Advance the locn object to the next item

	// If we have a leaf node, we don't need to worry about
	// traversing into children ... only need to worry about
	// going back up the tree.
	if (node->isLeaf) {
		// didn't visit the last node last time.
		if (lastPos> 0) {
			locn.second = lastPos - 1;
			return true;
		}
		goUp = (lastPos == 0);
	}

	// Not a leaf, therefore need to worry about traversing
	// into child nodes.
	else {
		node = node->loadChild(lastPos, _dataFile);
		while ((btree_node*)node != 0 && !node->isLeaf) {
			node = node->loadChild(node->objCount, _dataFile);
		}
		if ((btree_node*)node == 0) {

			return false;
		}
		locn.first = node;
		locn.second = node->objCount - 1;
		return true;
	}

	// Finished off a leaf, therefore need to go up to
	// a parent.
	if (goUp) {
		size_t childNo = node->childNo;
		node = node->parent;

		while ((btree_node*)node != 0 && childNo == 0) {
			childNo = node->childNo;
			node = node->parent;
		}
		if ((btree_node*)node != 0) {
			locn.first = node;
			locn.second = childNo - 1;
			return true;
		}
	}
	//reach the fist locn
	--locn.second = -1;
	return ret;
}

// This method flushes all loaded nodes to the file and
// then unloads the root node' children. So not only do we commit
// everything to file, we also free up most memory previously
// allocated.
void btreedb::flush() {

	//write back the fileHead and dirtypage
	share_rwlock lock(_flushLock, 1);
	commit();
	
	if (_root) {
		delete _root;
		_root = 0;
	}
	return;
}
