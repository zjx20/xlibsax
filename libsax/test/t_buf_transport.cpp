/*
 * t_buf_transport.cpp
 *
 *  Created on: 2011-8-16
 *      Author: x_zhou
 */

#include "gtest/gtest.h"
#include "gen-cpp/test_types.h"
#include "boost/shared_ptr.hpp"
#include "sax/net/t_buffer_transport.h"
#include "sax/net/buffer.h"
#include "protocol/TBinaryProtocol.h"

using namespace sax;

TEST(t_buf_transport, read_and_write)
{
	test::TestReq req;
	req.req = "hello";

	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	boost::shared_ptr< sax::TBufferTransport > _buffer_transport(new sax::TBufferTransport());
	boost::shared_ptr< apache::thrift::protocol::TBinaryProtocol >
		_protocol(new apache::thrift::protocol::TBinaryProtocol(_buffer_transport));

	_buffer_transport->set_buffer(&buf, false);
	req.write(_protocol.get());
	_buffer_transport->flush();

	buf.flip();

	_buffer_transport->set_buffer(&buf, true);
	test::TestReq req2;
	req2.read(_protocol.get());

	EXPECT_TRUE(req == req2);
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));


    return RUN_ALL_TESTS();
}
