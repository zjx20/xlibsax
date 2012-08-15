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
	virtual void init(uint32_t number_of_queues) = 0;
	virtual uint32_t dispatch(int32_t event_type, uint64_t shard_key) = 0;
};

class single_dispatcher : public dispatcher_base
{
public:
	inline void init(uint32_t) {}
	inline uint32_t dispatch(int32_t, uint64_t) { return 0; }
};

class default_dispatcher : public dispatcher_base
{
public:
	default_dispatcher() : _curr(0), _queues(0) {}
	~default_dispatcher() {}

	void init(uint32_t number_of_queues) { _queues = number_of_queues; }

	uint32_t dispatch(int32_t, uint64_t)
	{
		if (++_curr >= _queues) _curr = 0;
		return _curr;
	}

private:
	uint32_t _curr;
	uint32_t _queues;
};

} // namespace

#endif /* DISPATCHER_H_ */
