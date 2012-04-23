#ifndef __DICT_H_QS__
#define __DICT_H_QS__

/**
 * @file dict.h
 * @brief manipulate dictionary (map) with pointers
 *
 * @author Qingshan
 * @date 2011-1-28
 */

#include "hashmap.h"

namespace sax {

template <typename key_t, typename val_t>
class dict
{
public:
	typedef dict<key_t, val_t> self_type;
	typedef key_t key_type;
	typedef val_t value_type;
	
	typedef sax::hashmap<key_t, val_t *> map_t;
	typedef typename map_t::iterator iter_t;
	
	val_t *add(const key_t &key)
	{
		val_t *ptr = new val_t;
		if (ptr) {
			if (!_map.insert(std::make_pair(key, ptr)).second)
			{
				delete ptr;
				return 0;
			}
		}
		return ptr;
	}
	
	val_t *add(const key_t &key, const val_t &val)
	{
		val_t *ptr = new val_t(val);
		if (ptr) {
			if (!_map.insert(std::make_pair(key, ptr)).second)
			{
				delete ptr;
				return 0;
			}
		}
		return ptr;
	}
	
	void sub(iter_t &it)
	{
		val_t *ptr = it->second;
		if (ptr) delete ptr;
		_map.erase(it);
	}
	
	bool get(const key_t &key, iter_t &it)
	{
		it = _map.find(key);
		return (it != _map.end());
	}
	
	val_t *get(const key_t &key)
	{
		iter_t it = _map.find(key);
		return (it != _map.end() ? it->second : 0);
	}

	val_t *swap(const key_t &key, val_t *neu = 0)
	{
		iter_t it = _map.find(key);
		if (it != _map.end()) return 0;
		
		val_t *old = it->second;
		it->second = neu; return old;
	}

	void clear()
	{
		iter_t it = _map.begin();
		for (; it != _map.end(); it++)
		{
			val_t *ptr = it->second;
			if (ptr) delete ptr;
		}
		_map.clear();
	}

	inline size_t size() const { return _map.size(); }

	inline iter_t end() const { return _map.end(); }
	
	inline iter_t begin() const { return _map.begin(); }
	
	inline  dict() {}
	
	inline ~dict() { this->clear(); }

	// copy constructor:
	dict(const self_type &other)
	{
		iter_t it = other.begin();
		for (; it != other.end(); it++) {
			this->add(it->first, *(it->second));
		}
	}
	
	// assign value:
	self_type &operator=(const self_type &other)
	{
		if (this != &other)
		{
			this->clear();
			iter_t it = other.begin();
			for (; it != other.end(); it++) {
				this->add(it->first, *(it->second));
			}
		}
		return *this;
	}

private:
	mutable map_t _map; // the core container
	
};

} // namespace

#endif//__DICT_H_QS__
