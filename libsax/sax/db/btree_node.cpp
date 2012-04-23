#include "btree_node.h"

using namespace sax;
btree_node::btree_node(
		CbFileHeader& fileHeader, g_share_t* fileLock, size_t& activeNum, cmpfunc *ccc) :
	objCount(0), fpos(-1), isLeaf(true), isLoaded(false), isDirty(1),
			childNo((size_t)-1), _fh(fileHeader), _maxKeys(_fh.maxKeys),
			_pageSize(_fh.pageSize), _overFlowSize(_fh.pageSize),
			_fileLock(fileLock), activeNodeNum(activeNum) {
	_comp = ccc;
	_overflowAddress = -1;
	_overflowPageCount = 0;

	keys = new char*[_fh.maxKeys];
	values = new char*[_fh.maxKeys];
	memset(keys, 0, sizeof(char*)*_fh.maxKeys);
	memset(values, 0, sizeof(char*)*_fh.maxKeys);
	children = new btree_node*[_fh.maxKeys+1];
	for (size_t i=0; i<_fh.maxKeys+1; i++)
		children[i] = NULL;

	//cout<<"activeNodeNum: "<<activeNodeNum<<endl;
}

// Read a node page to initialize this node from the disk
bool btree_node::read(FILE* f) {

	if (!f) {
		return false;
	}

	//cout<<"read from fpos "<<fpos<<endl;
	share_rwlock lock(_fileLock, 1);
	
	if (isLoaded) {
		return true;
	}

	// get to the right location
	if (0 != fseek(f, fpos, SEEK_SET)) {
		return false;
	}

	char *pBuf = new char[_pageSize];
	if (1 != fread(pBuf, _pageSize, 1, f)) {
		if (pBuf)
			delete pBuf;
		pBuf = 0;
		return false;
	}

	char *p = pBuf;
	size_t tsz = 0;

	// read the leaf flag and the object count
	byte leafFlag = 0;
	memcpy(&leafFlag, p, sizeof(byte));
	p += sizeof(byte);
	tsz += sizeof(byte);
	memcpy(&objCount, p, sizeof(size_t));
	p += sizeof(size_t);
	tsz += sizeof(size_t);
	isLeaf = (leafFlag == 1);

	// read the addresses of the child pages
	if (objCount> 0 && !isLeaf) {
		long* childAddresses = (long*)p;//new long[objCount + 1];
		//memcpy(childAddresses, p, sizeof(long)*(objCount+1));
		p += sizeof(long)*(objCount+1);
		tsz += sizeof(long)*(objCount+1);

		//Only allocate childnode when the node is no a leaf node.
		if ( !isLeaf) {
			if( !children )
				children = new btree_node*[_fh.maxKeys+1];
			for (size_t i=0; i<_fh.maxKeys+1; i++)
				children[i] = NULL;
			for (size_t i = 0; i <= objCount; i++) {
				if (children[i] == 0) {
					children[i] = new btree_node(_fh, _fileLock, activeNodeNum, _comp);
				}
				children[i]->fpos = childAddresses[i];
				children[i]->childNo = i;
			}
		}
		//delete[] childAddresses;
		childAddresses = 0;
	}

	memcpy(&_overflowAddress, p, sizeof(long));
	p += sizeof(long);
	tsz += sizeof(long);
	memcpy(&_overflowPageCount, p, sizeof(size_t));
	p += sizeof(size_t);
	tsz += sizeof(size_t);

	char *povfl=0;

	bool first_ovfl = true;

	//cout<<" read overFlowAddress="<<_overflowAddress<<endl;

	//read the key/vaue pairs
	//cout<<"objCount"<<objCount<<endl;
	for (size_t i = 0; i < objCount; i++) {

		//cout<<" read data idx="<<i<<endl;
		size_t ksize, vsize;
		memcpy(&ksize, p, sizeof(size_t));

		//cout<<"ksize"<<ksize<<endl;
		//read from overflow page
		if (ksize == 0) {
			assert(first_ovfl);
			if (first_ovfl)
				first_ovfl =false;
			if ( !povfl) {
				//cout<<"read overflowaddress="<<_overflowAddress<<" | "<<_overflowPageCount<<endl;

				povfl = new char[_pageSize*_overflowPageCount];
				if (0 != fseek(f, _overflowAddress, SEEK_SET)) {
					if (pBuf)
						delete pBuf;
					delete povfl;
					return false;
				}
				if (1 != fread(povfl, _pageSize*_overflowPageCount, 1, f)) {
					if (pBuf)
						delete pBuf;
					delete povfl;
					return false;
				}
			}
			//p = povfl + np*_pageSize;
			p= povfl;
			memcpy(&ksize, p, sizeof(size_t));
			assert(ksize != 0);
			//is_incr_page = true;
		}
		
		p += sizeof(size_t);
		
		keys[i] = new char[ksize + sizeof(size_t)];
		char *tkey = keys[i];
		memcpy(tkey, &ksize, sizeof(size_t));
		tkey += sizeof(size_t);
		memcpy(tkey, p, ksize);
		p += ksize;

		memcpy(&vsize, p, sizeof(size_t));
		p += sizeof(size_t);

		//if value is of NULLType, the vsize is 0
		if (vsize != 0) {
			values[i] = new char[vsize + sizeof(size_t)];
			char *tval = values[i];
			memcpy(tval, &vsize, sizeof(size_t));
			tval += sizeof(size_t);
			memcpy(tval, p, vsize);
			p += vsize;
		}

	}

	if (povfl) {
		delete [] povfl;
		povfl = 0;
	}
	delete [] pBuf;
	pBuf = 0;
	isLoaded = true;
	isDirty = false;

	//increment avtiveNodeNum
	++activeNodeNum;
	return true;
}

bool btree_node::write(FILE* f) {

	// If it is not loaded, it measn that we haven't been changed it ,
	// so we can say that the flush was successful.
	if (!isDirty) {
		return false;
	}
	if (!isLoaded) {
		return false;
	}
	if (!f) {
		return false;
	}

	size_t tsz=0;

	//cout<<"_pageSize"<<_pageSize<<endl;
	char* pBuf = new char[_pageSize];
	memset(pBuf, 0, _pageSize);
	char* p = pBuf;

	//cout<<"write fpos="<<fpos<<endl;
	share_rwlock lock(_fileLock, 1);

	// get to the right location
	if (0 != fseek(f, fpos, SEEK_SET)) {
		//assert(false);
		return false;
	}

	// write the leaf flag and the object count
	byte leafFlag = isLeaf ? 1 : 0;

	memcpy(p, &leafFlag, sizeof(byte));
	p += sizeof(byte);
	tsz += sizeof(byte);

	//cout<<"write objCount= "<<objCount<<endl;
	memcpy(p, &objCount, sizeof(size_t));
	p += sizeof(size_t);
	tsz += sizeof(size_t);

	// write the addresses of the child pages
	if (objCount> 0 && !isLeaf) {
		long* childAddresses = (long*)p;//new long[objCount + 1];
		tsz += ((objCount+1)*sizeof(long));
		assert(tsz < _pageSize);
		for (size_t i=0; i<=objCount; i++) {
			childAddresses[i] = children[i]->fpos;
			//cout<<"write child fpos="<<childAddresses[i]<<endl;
		}
		//memcpy(p, childAddresses, sizeof(long)*(objCount+1));
		p += ((objCount+1)*sizeof(long));
		//delete [] childAddresses;
		childAddresses = 0;

	}

	size_t ovfloff = tsz;
	memcpy(p, &_overflowAddress, sizeof(long));
	p += sizeof(long);
	tsz += sizeof(long);
	memcpy(p, &_overflowPageCount, sizeof(size_t));
	p += sizeof(size_t);
	tsz += sizeof(size_t);

	assert(tsz+sizeof(size_t) <= _pageSize);
	size_t np =1;

	bool first_ovfl = true;
	for (size_t i=0; i<objCount; i++) {
		//cout<<"idx = "<<i<<endl;
		
		size_t ksize, vsize;
		char *ptr = keys[i];
		char *ptr1 = values[i];
		
		memcpy(&ksize, ptr, sizeof(size_t));
		memcpy(&vsize, ptr1, sizeof(size_t));

		
		size_t esize = 2*sizeof(size_t)+ksize+vsize;

		//when overflowing occurs, append the overflow buf
		if (tsz+esize+sizeof(size_t) > np*_pageSize) {

			assert(size_t(p - pBuf) < np*_pageSize);
			if (first_ovfl) {
				tsz = _pageSize;
				first_ovfl = false;
			}
			np = (tsz+esize+sizeof(size_t)-1 )/_pageSize+1;

			char *temp = new char[np*_pageSize];
			memset(temp, 0xff, np*_pageSize);
			memcpy(temp, pBuf, tsz);
			delete [] pBuf;
			pBuf = 0;
			pBuf = temp;

			p = pBuf+tsz;
		}
		memcpy(p, ptr, ksize + sizeof(size_t));
		p += (ksize + sizeof(size_t));
		memcpy(p, ptr1, vsize + sizeof(size_t));
		p += (vsize + sizeof(size_t));
		tsz += esize;
	}

	//no overflow
	if (np <= 1) {
		if (1 != fwrite(pBuf, _pageSize, 1, f) ) {
			return false;
		}
	} else {
		//oveflow
		//cout<<"writing overflow!!!!"<<endl;
		if (_overflowAddress <0 || _overflowPageCount < np-1) {
			_overflowAddress = sizeof(CbFileHeader) +_pageSize *(_fh.nPages +_fh.oPages);
			_fh.oPages += (np-1);
		}
		_overflowPageCount = np-1;
		memcpy(pBuf+ovfloff, &_overflowAddress, sizeof(long));
		memcpy(pBuf+ovfloff+sizeof(long), &_overflowPageCount, sizeof(size_t));
		if (1 != fwrite(pBuf, _pageSize, 1, f) ) {
			return false;
		}
		if (0 != fseek(f, _overflowAddress, SEEK_SET)) {
			return false;
		} else {
			if (1 != fwrite(pBuf+_pageSize, _pageSize*(np-1), 1, f) ) {
				return false;
			}
		}
	}
	delete [] pBuf;
	pBuf = 0;
	isDirty = false;
	return true;
}


// Unload a child, which means that we get rid of all
// children in the children vector.
void btree_node::unload() {
	if (isLoaded) {
		if ( !isLeaf) {
			if(children)
			{
				for (size_t i=0; i<objCount+1; i++) {
					if (children[i]) {
						delete children[i];
						children[i] = 0;
					}
				}
			}
		}
		objCount = 0;
		_clear_kv();
		isLoaded = false;

		--activeNodeNum;
		parent = 0;
	}
}

void btree_node::unloadself() {
	if (isLoaded) {
		objCount = 0;
		_clear_kv();
		delete [] children;
		children = NULL;
		isLoaded = false;

		--activeNodeNum;
		parent = 0;
	}
}

bool btree_node::delFromLeaf(size_t objNo) {
	bool ret = isLeaf;
	if (ret) {
		delete [] keys[objNo];
		delete [] values[objNo];
		for (size_t i = objNo + 1; i < objCount; i++) {
			keys[i-1] = keys[i];
			values[i-1] = values[i];
		}
		keys[objCount - 1] = 0;
		values[objCount - 1] = 0;
		setCount(objCount - 1);
	}
	isDirty = 1;
	return ret;
}

// Find the position of the object in a node. If the key is at pos
// the function returns (pos, ECP_INTHIS). If the key is in a child to
// the left of pos, the function returns (pos, ECP_INLEFT). If the node
// is an internal node, the function returns (objCount, ECP_INRIGHT).
// Otherwise, the function returns ((size_t)-1, false).
// The main assumption here is that we won't be searching for a key
// in this node unless it (a) is not in the tree, or (b) it is in the
// subtree rooted at this node.

KEYPOS btree_node::findPos(const void *key, size_t klen) {

	KEYPOS ret((size_t)-1, CCP_NONE);
	size_t i = 0;
	while (i<objCount) {
		size_t kl = ((size_t*)(keys[i]))[0];
		int compVal = _comp(key, klen, keys[i] + sizeof(size_t), kl);
		if (compVal == 0) {
			return KEYPOS(i, CCP_INTHIS);
		} else if (compVal < 0) {
			if (isLeaf) {
				return ret;
			} else {
				return KEYPOS(i, CCP_INLEFT);
			}
		}
		++i;
	}
	if (!isLeaf) {
		return KEYPOS(i - 1, CCP_INRIGHT);
	}
	return ret;
}
