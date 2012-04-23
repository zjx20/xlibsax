#ifndef __STACK_H_QS__
#define __STACK_H_QS__

/**
 * a thread-safe stack implemented by array
 */

#include <sax/sysutil.h>
#include "nocopy.h"

namespace sax {

template<class T>
class stack : public nocopy
{
public:
	inline stack(uint32_t cap=1024): _cap(cap), _cur(0)
	{
		_msg = new T[cap];
	}
	inline ~stack() 
	{
		delete[] _msg;
	}
	bool push(const T &one)
	{
		auto_mutex lock(&_mtx);
		if (_cur >= _cap) return false;
		
		_msg[_cur++] = one;
		return true;
	}
	bool pop(T &one)
	{
		auto_mutex lock(&_mtx);
		if (_cur==0) return false;
		
		one = _msg[--_cur];
		return true;
	}

private:
	T *_msg; ///> container
	uint32_t _cap; ///> size
	uint32_t _cur; ///> cursor
	mutex_type _mtx;
};

} //namespace

#endif//__STACK_H_QS__
