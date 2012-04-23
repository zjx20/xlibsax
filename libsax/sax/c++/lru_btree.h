#ifndef __LRU_BTREE_H_QS__
#define __LRU_BTREE_H_QS__

/**
 * @file lru_btree.h Template cache with an LRU removal policy
 * @remark implemented by btree for removing range of items
 * @author Qingshan (qluo.cn@gmail.com)
 * @version 0.15
 * @date March, 2011
 **/

#include <vector>
#include <list>
#include <stx/btree_map.h>

namespace sax {

// A linked list (std::list) is used for element container:
// First entry is element which has been used most recently.
// Last entry is element which has been used least recently.

template<class key_t, class val_t>
class LRU_btree
{
public:
	typedef LRU_btree<key_t, val_t> self_type;
	typedef key_t key_type;
	typedef val_t value_type;
	
	typedef std::list<std::pair<key_t, val_t> > list_t;
	typedef typename list_t::iterator iter_t;
	typedef stx::btree_map<key_t, iter_t> map_t;

	inline LRU_btree():_cap(1024) {}

	inline LRU_btree(size_t cap):_cap(cap) {}

	virtual ~LRU_btree() { this->clear(); }

	void resize(size_t cap)
	{
		this->clear();
		_cap = cap;
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
		_idx.erase(key);
		return true;
	}
	
	bool remove(const key_t &kmin, const key_t &kmax) {
		typename map_t::iterator i = _idx.lower_bound(kmin);
		if (i == _idx.end()) return false;
		
		if (i->first < kmin) i++;
		std::vector<key_t> hack;
		for (; i != _idx.end(); i++)
		{
			if (i->first > kmax) break;
			
			typename list_t::iterator n = i->second;
			onErase(n->first, n->second);
			
			_dat.erase(n);
			hack.push_back(i->first);
		}
		if (hack.size()==0) return false;
		
		typename std::vector<key_t>::iterator j;
		for (j=hack.begin(); j!=hack.end(); j++)
		{
			_idx.erase(*j);
		}
		return (_idx.size()==_dat.size());
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
	LRU_btree(const self_type &other);
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

#endif //__LRU_BTREE_H_QS__

