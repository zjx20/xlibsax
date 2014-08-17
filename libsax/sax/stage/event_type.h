/*
 * event_type.h
 *
 *  Created on: 2012-8-1
 *      Author: x
 */

#ifndef EVENT_TYPE_H_
#define EVENT_TYPE_H_

#include "sax/compiler.h"
#include "sax/os_types.h"

namespace sax {

struct event_type {
public:
	enum
	{
		USER_TYPE_START = 10240
	};
public:
	inline int32_t get_type() const {return type_id;}
	virtual void destroy() = 0;
	virtual ~event_type() {}
protected:
	inline event_type(int32_t id=-1) : type_id(id) {}
	int32_t type_id;
};

template <int32_t TID, typename REAL_TYPE>
class sax_event_base : public event_type
{
public:
	enum {ID = TID};
	virtual ~sax_event_base() {}

	void destroy()
	{
		static_cast<const REAL_TYPE*>(this)->~REAL_TYPE();
	}

protected:
	inline sax_event_base() : event_type(TID) {}
};

template <int32_t TID, typename REAL_TYPE>
class user_event_base : public event_type
{
public:
	enum {ID = TID + event_type::USER_TYPE_START};
	virtual ~user_event_base() {}

	void destroy()
	{
		static_cast<const REAL_TYPE*>(this)->~REAL_TYPE();
	}

protected:
	inline user_event_base() : event_type(TID) {}
};

} // namespace

#endif /* EVENT_TYPE_H_ */
