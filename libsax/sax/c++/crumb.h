#ifndef __CRUMB_H_QS__
#define __CRUMB_H_QS__

/**
 * @file crumb.h
 * @brief the key-value version of expire.h
 * It has implemented a template class for crumb helper.
 *
 * Note that it is different from the old implementation:
 *  - we should call purge() outside: before add(), after sub()
 *  - overdue is an customized parameter rather than a macro.
 *
 * @author Qingshan
 * @date 2011.1.7
 */

#include "hashmap.h"
#include <list>
#include <time.h>

namespace sax {

template <typename T>
void copy_object(const T *src, T *dst)
{
	*dst = *src;
}

template <typename KEY, typename VAL>
class crumb
{
public:
	typedef crumb<KEY, VAL> self_type;
	typedef KEY key_type;
	typedef VAL value_type;
	
	typedef struct {time_t tick; KEY key; VAL val;} DataTriple;
	typedef std::list<DataTriple> DataList;
	typedef typename DataList::iterator DataIter;
	typedef sax::hashmap<KEY, DataIter> IndexDict;
	
	crumb(int sec=60, size_t cap=1024) : _overdue(sec)
	{
		if (cap) _idx.rehash(cap);
	}
	
	virtual ~crumb() {
		_dat.clear();
		_idx.clear();
	}
	
	inline void init(int sec, size_t cap)
	{
		_overdue = sec;
		if (cap) _idx.rehash(cap);
	}
	
	bool add(const KEY &key, const VAL &val)
	{
		typename IndexDict::iterator it = _idx.find(key);
		if (it == _idx.end()) {
			std::pair<typename IndexDict::iterator, bool> ret;
			ret = _idx.insert(std::make_pair(key, _dat.end()));
			if (!ret.second) return false;
			else it = ret.first;
			
			DataTriple one = {time(NULL), key, val};
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
	
	bool sub(const KEY &key, VAL *val)
	{
		typename IndexDict::iterator it = _idx.find(key);
		if (it == _idx.end()) return false;
		
		if (val) *val = it->second->val;
		onErase(key, it->second->val);
		
		_dat.erase(it->second);
		_idx.erase(it);
		return true;
	}
	
	bool get(const KEY &key, VAL *val)
	{
		typename IndexDict::iterator it = _idx.find(key);
		if (it == _idx.end()) return false;
		
		if (val) *val = it->second->val;
		return true;
	}
	
	VAL *find(const KEY &key)
	{
		typename IndexDict::iterator it = _idx.find(key);
		if (it == _idx.end()) return (VAL *) 0;
		
		return &it->second->val;
	}
	
	// Compare to find(), it updates time stamp meanwhile.
	VAL *touch(const KEY &key)
	{
		typename IndexDict::iterator it = _idx.find(key);
		if (it == _idx.end()) return (VAL *) 0;
		
		DataIter ii = it->second;
		ii->tick = time(NULL); // update
		_dat.splice(_dat.end(), _dat, ii);
		return &ii->val;
	}
	
	size_t purge()
	{
		DataIter ii = _dat.begin();
		size_t cnt = 0;
		while (ii != _dat.end())
		{
			if (time(NULL) - ii->tick < _overdue) break;
			onPurge(ii->key, ii->val);
			cnt++;
			_idx.erase(ii->key);
			_dat.erase(ii++); 
		}
		return cnt;
	}
	
	inline size_t size() const { return _idx.size(); }

protected:
	crumb(const crumb<KEY, VAL> &other);
	crumb<KEY, VAL> &operator=(const crumb<KEY, VAL> &other);

protected:
	virtual void onErase(const KEY &key, const VAL &val) {}
	virtual void onPurge(const KEY &key, const VAL &val) {}

private:
	int _overdue;
	DataList _dat;
	IndexDict _idx;
};

} // namespace

#endif//__CRUMB_H_QS__
