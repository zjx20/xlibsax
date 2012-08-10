/*
 * event_type.h
 *
 *  Created on: 2012-8-1
 *      Author: x
 */

#ifndef EVENT_TYPE_H_
#define EVENT_TYPE_H_

#include "sax/slabutil.h"

namespace sax {

struct event_type {
public:
	enum
	{
		USER_TYPE_START = 10240
	};
public:
	inline int get_type() const {return type_id;}
	virtual void destroy() = 0;
	virtual uint32_t size() = 0;
	virtual event_type* copy_to(void* ptr) = 0;
	~event_type() {}	// declare as non-virtual intentionally
protected:
	inline event_type(int32_t id=-1) : type_id(id) {}
	int32_t type_id;
};

template <int TID, typename REAL_TYPE>
class sax_event_base : public event_type
{
public:
	enum {ID = TID};
	virtual ~sax_event_base() {}

	static REAL_TYPE* new_event()
	{
		return SLAB_NEW(REAL_TYPE);
	}

	void destroy()
	{
		SLAB_DELETE(REAL_TYPE, this);
	}

	uint32_t size()
	{
		return sizeof(REAL_TYPE);
	}

	event_type* copy_to(void* ptr)
	{
		const REAL_TYPE* obj = static_cast<const REAL_TYPE*>(this);
		return static_cast<event_type*>(new (ptr) REAL_TYPE(*obj));
	}

protected:
	inline sax_event_base() : event_type(TID) {}
};

template <int TID, typename REAL_TYPE>
class user_event_base : public event_type
{
public:
	enum {ID = TID};
	virtual ~user_event_base() {}

	static REAL_TYPE* new_event()
	{
		return SLAB_NEW(REAL_TYPE);
	}

	void destroy()
	{
		SLAB_DELETE(REAL_TYPE, this);
	}

	uint32_t size()
	{
		return sizeof(REAL_TYPE);
	}

	event_type* copy_to(void* ptr)
	{
		const REAL_TYPE* obj = static_cast<const REAL_TYPE*>(this);
		return static_cast<event_type*>(new (ptr) REAL_TYPE(*obj));
	}

protected:
	inline user_event_base() : event_type(TID) {}
};

} // namespace

#endif /* EVENT_TYPE_H_ */
