/*
 * dispatcher.h
 *
 *  Created on: 2012-8-1
 *      Author: x
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include "sax/os_types.h"
#include "event_type.h"

namespace sax {

class dispatcher_base
{
public:
	virtual ~dispatcher_base() {}
	virtual void init(int32_t number_of_queues) = 0;
	virtual int32_t dispatch(const event_type* ev) = 0;
};

class single_dispatcher : public dispatcher_base
{
public:
	inline void init(int32_t queues) {}
	inline int32_t dispatch(const event_type* ev) { return 0; }
};

class default_dispatcher : public dispatcher_base
{
public:
	default_dispatcher() : _curr(0), _queues(0) {}
	~default_dispatcher() {}

	void init(int32_t queues) { _queues = queues; }

	int32_t dispatch(const event_type* ev)
	{
		++_curr;
		if (_curr >= _queues) _curr = 0;
		return _curr;
	}

private:
	int32_t _curr;
	int32_t _queues;
};

} // namespace

#endif /* DISPATCHER_H_ */
