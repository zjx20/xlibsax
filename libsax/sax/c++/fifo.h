#ifndef __FIFO_H_QS__
#define __FIFO_H_QS__

/**
 * a thread-safe fifo queue implemented by array
 */

#include <sax/sysutil.h>
#include "nocopy.h"

namespace sax {

template<class T>
class fifo : public nocopy
{
public:
	inline fifo(uint32_t cap=1024):
		_cap(cap), _wsem(cap, cap), _rsem(cap, 0)
	{
		_msg = new T[cap];
		_wid = _rid = 0;
	}
	inline ~fifo() 
	{
		delete[] _msg;
	}
	bool push_back(const T &one, double sec)
	{
		if (_wsem.wait(sec))
		{
			auto_mutex lock(&_wmtx);
			_msg[_wid++] = one;
			if (_wid >= _cap) _wid = 0;
			_rsem.post();
			return true;
		}
		return false;
	}
	bool pop_front(T &one, double sec)
	{
		if (_rsem.wait(sec))
		{
			auto_mutex lock(&_rmtx);
			one = _msg[_rid++];
			if (_rid >= _cap) _rid = 0;
			_wsem.post();
			return true;
		}
		return false;
	}

protected:
	T *_msg; ///> container
	uint32_t _cap; ///> size
	
	uint32_t _wid; ///> cursor for writing
	uint32_t _rid; ///> cursor for reading

private:
	mutex_type _wmtx;
	mutex_type _rmtx;
	sema_type  _wsem;
	sema_type  _rsem;
};

} //namespace

#endif//__FIFO_H_QS__
