/*
 * t_thrift_rpc_serial.cpp
 *
 *  Created on: 2011-8-17
 *      Author: X
 */

#include "gtest/gtest.h"
#include "gen-cpp/test_types.h"
#include "gen-cpp/Test.h"
#include "sax/net/buffer.h"
#include "sax/net/t_buffer_transport.h"
#include "protocol/TBinaryProtocol.h"
#include "boost/shared_ptr.hpp"

using namespace apache::thrift::protocol;
using namespace sax;

TEST(thrift_rpc_args, read) {
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	boost::shared_ptr<sax::TBufferTransport> buffer_transport(
			new sax::TBufferTransport());
	boost::shared_ptr<TBinaryProtocol> protocol(
			new TBinaryProtocol(buffer_transport));

	uint8_t req_dump[] = { 0x000, 0x000, 0x000, 0x021, 0x080, 0x001, 0x000,
			0x001, 0x000, 0x000, 0x000, 0x004, 0x074, 0x065, 0x073, 0x074,
			0x000, 0x000, 0x000, 0x000, 0x00C, 0x000, 0x001, 0x00B, 0x000,
			0x001, 0x000, 0x000, 0x000, 0x005, 0x068, 0x065, 0x06C, 0x06C,
			0x06F, 0x000, 0x000 };

	buf.put(req_dump, sizeof(req_dump));
	buf.flip();

	buffer_transport->set_buffer(&buf, true);
	std::string name;
	TMessageType type;
	int32_t seqid = 0;
	protocol->readMessageBegin(name, type, seqid);
	EXPECT_EQ(name, "test");
	EXPECT_EQ(type, T_CALL);
	EXPECT_EQ(seqid, 0);

	test::Test_test_args args;
	args.read(protocol.get());
	EXPECT_EQ(args.req.req, "hello");

	protocol->readMessageEnd();

	EXPECT_EQ(buf.remaining(), (size_t)0);
}

TEST(thrift_rpc_result, write) {
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	boost::shared_ptr<sax::TBufferTransport> buffer_transport(
			new sax::TBufferTransport());
	boost::shared_ptr<TBinaryProtocol> protocol(
			new TBinaryProtocol(buffer_transport));

	uint8_t res_dump[] = { 0x000, 0x000, 0x000, 0x021, 0x080, 0x001, 0x000,
			0x002, 0x000, 0x000, 0x000, 0x004, 0x074, 0x065, 0x073, 0x074,
			0x000, 0x000, 0x000, 0x000, 0x00C, 0x000, 0x000, 0x00B, 0x000,
			0x001, 0x000, 0x000, 0x000, 0x005, 0x077, 0x06F, 0x072, 0x06C,
			0x064, 0x000, 0x000 };

	buffer_transport->set_buffer(&buf, false);

	std::string name = "test";
	TMessageType type = T_REPLY;
	int32_t seqid = 0;

	protocol->writeMessageBegin(name, type, seqid);

	test::Test_test_result result;
	result.success.res = "world";
	result.__isset.success = true;
	result.write(protocol.get());

	protocol->writeMessageEnd();
	buffer_transport->flush();

	buf.flip();
	EXPECT_EQ(buf.remaining(), sizeof(res_dump));

	uint8_t tmp[100];
	buf.get(tmp, buf.remaining());
	EXPECT_EQ(std::string((char*)res_dump, sizeof(res_dump)), std::string((char*)tmp, sizeof(res_dump)));
}

int main(int argc, char *argv[]) {
	testing::InitGoogleTest(&argc, argv);
	srand((unsigned) time(NULL));

	return RUN_ALL_TESTS();
}
