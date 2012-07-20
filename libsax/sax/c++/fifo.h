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
//		node* n = _whead;
//		while (n != NULL) {
//			node* tmp = n->next;
//			_allocator.deallocate(n);
//			n = tmp;
//		}  TODO
	}
	bool push_back(const T &one, double sec)
	{
		node* n = _allocator.allocate(1, NULL);
		if (UNLIKELY(n == NULL)) return false;

		_allocator.construct(&n->value, one);
		n->next = NULL;

		auto_lock<spin_type> lock(_wlock);
		if (_wtail != NULL) {
			_wtail->next = n;
			_wtail = n;
		}
		else {
			_whead = _wtail = n;
		}

		_sem.post();

		return true;
	}
	bool pop_front(T &one, double sec)
	{
		if (_sem.wait(sec)) {
			_rlock.enter();

			if (_rhead == NULL) {
				// _sem.wait() passed, always have element
				_wlock.enter();

				_rhead = _whead;
				_rtail = _wtail;
				_whead = NULL;
				_wtail = NULL;

				_wlock.leave();
			}

			node* n = _rhead;
			one = n->value;
			_allocator.destroy(n);
			_allocator.deallocate(n, 1);

			if (_rhead != _rtail) {
				_rhead = _rhead->next;
			}
			else {
				_rhead = _rtail = NULL;
			}

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

	typename Allocator::template rebind<node>::other _allocator;
};

} //namespace

#endif//__FIFO_H_QS__
