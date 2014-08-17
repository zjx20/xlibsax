#ifndef __FIFO_H_QS__
#define __FIFO_H_QS__

/**
 * a thread-safe fifo queue
 */

#include "sax/sysutil.h"
#include "sax/os_api.h"
#include "sax/compiler.h"
#include "nocopy.h"

namespace sax {

template <typename T>
class fifo : public nocopy
{
public:
	inline fifo(uint32_t cap = 1024) :
		_cap(cap), _hsem(_cap, 0), _tsem(_cap, _cap)
	{
		_head = _tail = 0;
		_queue = new T[_cap];
	}

	inline ~fifo()
	{
		delete[] _queue;
	}

	bool push_back(const T &one, double sec = 0)
	{
		if (_tsem.wait(sec)) {
			_tlock.enter();
			_queue[_tail++] = one;
			if (UNLIKELY(_tail >= _cap)) _tail = 0;
			_tlock.leave();

			_hsem.post();

			return true;
		}

		return false;
	}

	// sec < 0 wait forever; sec = 0 return immediately; sec > 0 wait sec seconds
	bool pop_front(T &one, double sec = 0)
	{
		if (_hsem.wait(sec)) {
			_hlock.enter();
			one = _queue[_head++];
			if (UNLIKELY(_head >= _cap)) _head = 0;
			_hlock.leave();

			_tsem.post();

			return true;
		}

		return false;
	}

	uint32_t get_capacity() { return _cap; }

private:
	uint32_t _cap;
	T* _queue;
	volatile uint32_t _head;
	volatile uint32_t _tail;

	sema_type _hsem;
	sema_type _tsem;
	spin_type _hlock;
	spin_type _tlock;
};

} //namespace

#endif//__FIFO_H_QS__
