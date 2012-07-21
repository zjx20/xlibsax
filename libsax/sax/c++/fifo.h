#ifndef __FIFO_H_QS__
#define __FIFO_H_QS__

/**
 * a thread-safe fifo queue
 */

#include "sax/sysutil.h"
#include "sax/compiler.h"
#include "nocopy.h"

namespace sax {

template<typename T, typename Allocator = std::allocator<T> >
class fifo : public nocopy
{
	struct node
	{
		T value;
		node* next;
	};

	typedef typename Allocator::template rebind<node>::other node_allocator_type;

public:
	inline fifo() : _sem((uint32_t)-1, 0)
	{
		_whead = NULL;
		_wtail = NULL;
		_rhead = NULL;
		_rtail = NULL;
	}

	inline ~fifo() 
	{
		if (_wtail != NULL) _wtail->next = _rhead;
		node* n = _whead;
		while (n != NULL) {
			node* tmp = n->next;
			(&n->value)->~T();
			node_allocator_type().deallocate(n, 1);
			n = tmp;
		}

		_whead = _wtail = NULL;
		_rhead = _rtail = NULL;
	}

	bool push_back(const T &one)
	{
		node* n = node_allocator_type().allocate(1, NULL);
		if (UNLIKELY(n == NULL)) return false;

		::new (&n->value) T(one);
		n->next = NULL;

		_wlock.enter();
		if (_wtail != NULL) {
			_wtail->next = n;
			_wtail = n;
		}
		else {
			_whead = _wtail = n;
		}
		_wlock.leave();

		_sem.post();

		return true;
	}

	// sec < 0 wait forever; sec = 0 return immediately; sec > 0 wait sec seconds
	bool pop_front(T &one, double sec = 0)
	{
		if (_sem.wait(sec)) {
			_rlock.enter();

			if (_rhead == NULL) {
				// _sem.wait() passed, there are always have elements to pop
				_wlock.enter();

				_rhead = _whead;
				_rtail = _wtail;
				_whead = _wtail = NULL;

				_wlock.leave();
			}

			node* n = _rhead;
			one = n->value;

			if (_rhead != _rtail) {
				_rhead = _rhead->next;
			}
			else {
				_rhead = _rtail = NULL;
			}

			(&n->value)->~T();
			node_allocator_type().deallocate(n, 1);

			_rlock.leave();

			return true;
		}
		return false;
	}

protected:
	node* _whead;
	node* _wtail;
	node* _rhead;
	node* _rtail;

private:
	spin_type _wlock;
	spin_type _rlock;
	sema_type _sem;
};

} //namespace

#endif//__FIFO_H_QS__
