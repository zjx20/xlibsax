/*
 * stage_mgr.h
 *
 *  Created on: 2012-8-1
 *      Author: x
 */

#ifndef STAGE_MGR_H_
#define STAGE_MGR_H_

#include "sax/c++/linkedlist.h"
#include "sax/sysutil.h"

namespace sax {

class stage;

class stage_mgr
{
public:
	static stage_mgr* get_instance();

	void register_stage(stage* st);
	void unregister_stage(stage* st);

	stage* get_stage_by_name(const char* name);

	void stop_all();

private:
	stage_mgr() {}
	~stage_mgr() {}

	spin_type _lock;
	linkedlist<stage> _stages;
};


} // namespace

#endif /* STAGE_MGR_H_ */
