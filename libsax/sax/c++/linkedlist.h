#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

#include "nocopy.h"

namespace sax {

/// A typical node should be defined as:
/// <pre>
/// struct NodeT  {
/// 	NodeT *_prev;
/// 	NodeT *_next;
/// };
/// </pre>

template <typename NodeT>
class linkedlist : public nocopy
{
public:
	typedef linkedlist<NodeT> self_type;
	typedef NodeT * node_pointer;
	
	linkedlist();
	void push_back(NodeT* node);
	void push_front(NodeT* node);
	void erase(NodeT* node);
	void splice(bool before, NodeT *dst, NodeT *src);
	void merge(const linkedlist<NodeT>& al);
	void merge(NodeT *head);

	NodeT* head() const { return _head; }
	NodeT* tail() const { return _tail; }

	bool empty() const { return !(_head && _tail); }
	
protected:
	void reset();

private:
	NodeT * _head;
	NodeT * _tail;
};

template <typename NodeT>
linkedlist<NodeT>::linkedlist()
{
	reset();
}

template <typename NodeT>
void linkedlist<NodeT>::reset()
{
	_head = _tail = 0;
}

template <typename NodeT>
void linkedlist<NodeT>::push_back(NodeT* node)
{
	if (!node) return;

	node->_prev = _tail;
	node->_next = 0;

	if (!_tail) {
		_head = node;
	}
	else {
		_tail->_next = node;
	}
	_tail = node;
}

template <typename NodeT>
void linkedlist<NodeT>::push_front(NodeT* node)
{
	if (!node) return;

	node->_prev = 0;
	node->_next = _head;

	if (!_head) {
		_tail = node;
	}
	else {
		_head->_prev = node;
	}
	_head = node;
}

template <typename NodeT>
void linkedlist<NodeT>::erase(NodeT* node)
{
	if (!node) return;

	if (node == _head) { // head
		_head = node->_next;
	}
	if (node == _tail) { // tail
		_tail = node->_prev;
	}

	if (node->_prev)
		node->_prev->_next = node->_next;
	if (node->_next)
		node->_next->_prev = node->_prev;

}

template <typename NodeT>
void linkedlist<NodeT>::splice(bool before, NodeT *where, NodeT *node)
{
	if (empty() || !where || !node || where == node) return;
	
	if (before) {
		if (where->_prev == node) return;
		this->erase(node);
		
		if (where->_prev)
			where->_prev->_next = node;
		node->_prev = where->_prev;
		
		node->_next = where;
		where->_prev = node;
		
		if (where == _head) _head = node;
	}
	else {
		if (where->_next == node) return;
		this->erase(node);
		
		if (where->_next)
			where->_next->_prev = node;
		node->_next = where->_next;
		
		node->_prev = where;
		where->_next = node;
		
		if (where == _tail) _tail = node;
	}
}

template <typename NodeT>
void linkedlist<NodeT>::merge(const linkedlist<NodeT>& al)
{
	if (al.empty()) return;

	if (!_tail) {
		_head = al.head();
	}
	else {
		_tail->_next = al.head();
		al.head()->_prev = _tail;
	}
	_tail = al.tail();
}

template <typename NodeT>
void linkedlist<NodeT>::merge(NodeT *head)
{
	if (!head) return;

	NodeT *tail = head;
	while (tail->_next) {
		tail = tail->_next;
	}

	if (!_tail) {
		_head = head;
		head->_prev = 0;
	}
	else {
		_tail->_next = head;
		head->_prev = _tail;
	}
	_tail = tail;
}

} //namespace

#endif//__LINKED_LIST_H__

