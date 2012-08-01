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
		BIZ_TYPE_START = 10240
	};
public:
	inline int get_type() const {return type_id;}
	virtual void destroy() = 0;
	virtual ~event_type() {}
protected:
	inline event_type(int32_t id=-1) : type_id(id) {}
	int32_t type_id;
};

template<int TID, typename REAL_TYPE>
class event_base : public event_type
{
public:
	enum {ID = TID};
	virtual ~event_base() {}

	static REAL_TYPE* new_event()
	{
		return SLAB_NEW(REAL_TYPE);
	}

	void destroy()
	{
		SLAB_DELETE(REAL_TYPE, this);
	}

protected:
	inline event_base() : event_type(TID) {}
};

} // namespace

#endif /* EVENT_TYPE_H_ */
