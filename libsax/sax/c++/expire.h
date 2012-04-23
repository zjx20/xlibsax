#ifndef __EXPIRE_H_2011__
#define __EXPIRE_H_2011__

/**
 * @file expire.h
 * @brief handle timeout with a list and a hash-map
 *
 * @author Qingshan
 * @date 2011-3-6
 */

#include "hashmap.h"
#include <list>
#include <time.h>

namespace sax {

template<typename KEY>
class expire
{
public:
	typedef expire<KEY> self_type;
	typedef KEY key_type;
	
	typedef struct {time_t tick; KEY key;} DataPair;
	typedef std::list<DataPair> DataList;
	typedef typename DataList::iterator DataIter;
	typedef sax::hashmap<KEY, DataIter> IndexDict;
	
	expire(int sec=60, size_t cap=1024) : _overdue(sec)
	{
		if (cap) _idx.rehash(cap);
	}
	
	virtual ~expire() {
		_dat.clear();
		_idx.clear();
	}

	inline void init(int sec, size_t cap)
	{
		_overdue = sec;
		if (cap) _idx.rehash(cap);
	}
	
	bool add(const KEY &key)
	{
		typename IndexDict::iterator it = _idx.find(key);
		if (it == _idx.end()) {
			std::pair<typename IndexDict::iterator, bool> ret;
			ret = _idx.insert(std::make_pair(key, _dat.end()));
			if (!ret.second) return false;
			else it = ret.first;
			
			DataPair one = {time(NULL), key};
			_dat.push_back(one); // append!
			it->second = --_dat.end(); // the true value
		}
		else { // just touch!
			DataIter ii = it->second;
			ii->tick = time(NULL); // update
			_dat.splice(_dat.end(), _dat, ii);
		}
		return true;
	}
	
	bool sub(const KEY &key)
	{
		typename IndexDict::iterator it = _idx.find(key);
		if (it == _idx.end()) return false;
		
		onErase(key);
		_dat.erase(it->second);
		_idx.erase(it);
		return true;
	}
	
	bool get(const KEY &key)
	{
		typename IndexDict::iterator it = _idx.find(key);
		return (it != _idx.end());
	}
	
	bool touch(const KEY &key)
	{
		typename IndexDict::iterator it = _idx.find(key);
		if (it == _idx.end()) return false;
		
		DataIter ii = it->second;
		ii->tick = time(NULL); // update
		_dat.splice(_dat.end(), _dat, ii);
		return true;
	}
	
	size_t purge()
	{
		DataIter ii = _dat.begin();
		size_t cnt = 0;
		while (ii != _dat.end())
		{
			if (time(NULL) - ii->tick < _overdue) break;
			onPurge(ii->key);
			cnt++;
			_idx.erase(ii->key);
			_dat.erase(ii++); 
		}
		return cnt;
	}
	
	inline size_t size() const { return _idx.size(); }

protected:
	expire(const expire<KEY> &other);
	expire<KEY> &operator=(const expire<KEY> &other);

protected:
	virtual void onErase(const KEY &key) {}
	virtual void onPurge(const KEY &key) {}

private:
	int _overdue;
	DataList _dat;
	IndexDict _idx;
};

} // namespace

#endif //__EXPIRE_H_2011__

