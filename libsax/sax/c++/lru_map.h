#ifndef __LRU_MAP_H_QS__
#define __LRU_MAP_H_QS__

/**
 * @file lru_map.h Template cache with an LRU removal policy
 * @author Qingshan (qluo.cn@gmail.com)
 * @version 0.12
 * @date March, 2011
 * @remark This code is ported from Patrick Audley's implmentation
 * (March 2006, version 1.2), lots of simplifications are done.
 **/

#include <list>
#include "hashmap.h"

namespace sax {

// A linked list (std::list) is used for element container:
// First entry is element which has been used most recently.
// Last entry is element which has been used least recently.

template<class key_t, class val_t>
class LRU_map
{
public:
	typedef LRU_map<key_t, val_t> self_type;
	typedef key_t key_type;
	typedef val_t value_type;
	
	typedef std::list<std::pair<key_t, val_t> > list_t;
	typedef typename list_t::iterator iter_t;
	typedef sax::hashmap<key_t, iter_t> map_t;

	inline LRU_map():_cap(1024) {}

	inline LRU_map(size_t cap): _cap(cap)
	{
		_idx.rehash(_cap);
	}

	virtual ~LRU_map() { this->clear();	}

	void resize(size_t cap)
	{
		this->clear();
		_cap = cap;
		_idx.rehash(_cap);
	}

	val_t *find(const key_t &key)
	{
		typename map_t::iterator i = _idx.find(key);
		if (i == _idx.end()) return NULL;
		
		typename list_t::iterator n = i->second;
		_dat.splice(_dat.begin(), _dat, n);
		return &(_dat.front().second);
	}

	void insert(const key_t &key, const val_t &val)
	{
		typename map_t::iterator i = _idx.find(key);
		if (i != _idx.end()) // found
		{ 
			typename list_t::iterator n = i->second;
			n->second = val;
			_dat.splice(_dat.begin(), _dat, n);
		}
		else if (_dat.size() >= _cap)
		{ 
			typename list_t::iterator n = _dat.end();
			--n; // move to the last element
			
			onPurge(n->first, n->second);
			_idx.erase(n->first);
			
			n->first = key;
			n->second = val;
			
			_dat.splice(_dat.begin(), _dat, n);
			_idx[key] = n;
		}
		else
		{
			_dat.push_front(std::make_pair(key, val));
			typename list_t::iterator n = _dat.begin();
			_idx[key] = n;
		}
	}

	bool remove(const key_t &key) {
		typename map_t::iterator i = _idx.find(key);
		if (i == _idx.end()) return false;
		
		typename list_t::iterator n = i->second;
		onErase(n->first, n->second);
		
		_dat.erase(n);
		_idx.erase(i);
		return true;
	}

	/// Random access to items
	inline iter_t begin()
	{
		return _dat.begin();
	}

	inline iter_t end()
	{
		return _dat.end();
	}

	inline size_t size()
	{
		return _idx.size();
	}

	/// Clear cache
	inline void clear()
	{
		_idx.clear();
		_dat.clear();
	}
	
protected:
	LRU_map(const self_type &other);
	self_type &operator=(const self_type &other);

protected:
	virtual void onErase(const key_t &key, const val_t &val) {}
	virtual void onPurge(const key_t &key, const val_t &val) {}
	
private:
	list_t _dat;
	map_t  _idx;
	size_t _cap;
};

} //namespace

#endif //__LRU_MAP_H_QS__

