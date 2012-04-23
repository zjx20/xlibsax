#ifndef __LRU_SET_H_QS__
#define __LRU_SET_H_QS__

/**
 * @file lru_set.h the "set" version of lru_map.h
 * @author Qingshan (qluo.cn@gmail.com)
 **/

#include <list>
#include "hashmap.h"

namespace sax {

// A linked list (std::list) is used for element container:
// First entry is element which has been used most recently.
// Last entry is element which has been used least recently.

template <class key_t>
class LRU_set
{
public:
	typedef LRU_set<key_t> self_type;
	typedef key_t key_type;
	
	typedef std::list<key_t> list_t;
	typedef typename list_t::iterator iter_t;
	typedef sax::hashmap<key_t, iter_t> map_t;

	inline LRU_set():_cap(1024) {}

	inline LRU_set(size_t cap): _cap(cap)
	{
		_idx.rehash(_cap);
	}

	virtual ~LRU_set() { this->clear();	}

	void resize(size_t cap)
	{
		this->clear();
		_cap = cap;
		_idx.rehash(_cap);
	}

	bool find(const key_t &key)
	{
		typename map_t::iterator i = _idx.find(key);
		if (i == _idx.end()) return false;
		
		typename list_t::iterator n = i->second;
		_dat.splice(_dat.begin(), _dat, n);
		return true;
	}

	void insert(const key_t &key)
	{
		typename map_t::iterator i = _idx.find(key);
		if (i != _idx.end()) // found
		{ 
			typename list_t::iterator n = i->second;
			_dat.splice(_dat.begin(), _dat, n);
		}
		else if (_dat.size() >= _cap)
		{ 
			typename list_t::iterator n = _dat.end();
			--n; // move to the last element
			
			onPurge(*n);
			_idx.erase(*n);
			*n = key;
			
			_dat.splice(_dat.begin(), _dat, n);
			_idx[key] = n;
		}
		else
		{
			_dat.push_front(key);
			typename list_t::iterator n = _dat.begin();
			_idx[key] = n;
		}
	}

	bool remove(const key_t &key) {
		typename map_t::iterator i = _idx.find(key);
		if (i == _idx.end()) return false;
		
		onErase(key);
		
		_dat.erase(i->second);
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
	LRU_set(const self_type &other);
	self_type &operator=(const self_type &other);

protected:
	virtual void onErase(const key_t &key) {}
	virtual void onPurge(const key_t &key) {}
	
private:
	list_t _dat;
	map_t  _idx;
	size_t _cap;
};

} //namespace

#endif //__LRU_SET_H_QS__

