/*
 * t_thrift_utility.cpp
 *
 *  Created on: 2011-8-16
 *      Author: x_zhou
 */

#include "gtest/gtest.h"
#include "sax/net/buffer.h"
#include "sax/net/thrift_utility.h"
#include "sax/net/thrift_task.h"
#include "gen-cpp/test_types.h"
#include "gen-cpp/test_struct_typeid.h"

using namespace test;
using namespace sax;

static void test_to_buffer(sax::buffer* buf, void* obj)
{
	buf->put((uint32_t)*((uint32_t*)obj));
}

static void* test_to_object(sax::buffer* buf)
{
	uint32_t* t_obj = new uint32_t;
	buf->get(*t_obj);
	return t_obj;
}

TEST(thrift_utility, t_register)
{
	sax::thrift_utility::thrift_register(123456, test_to_object, test_to_buffer);

	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	uint32_t test_num = 45678422;
	sax::thrift_utility::to_buffer(&buf, (void*)&test_num, 123456);
	buf.flip();

	uint32_t* test_num2 = (uint32_t*)sax::thrift_utility::to_object(&buf, 123456);
	ASSERT_TRUE(test_num2 != NULL);

	EXPECT_EQ(test_num, *test_num2);
	delete test_num2;
}

TEST(thrift_utility, auto_serial)
{
	TestReq* req = new TestReq();
	req->req = "world";

	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	sax::thrift_utility::to_buffer(&buf, (void*)req, TType::T_TEST_REQ);
	buf.flip();

	TestReq* req2 = (TestReq*)sax::thrift_utility::to_object(&buf, TType::T_TEST_REQ);
	ASSERT_TRUE(req2 != NULL);

	EXPECT_TRUE(*req == *req2);
	delete req2;
}

TEST(thrift_utility, auto_rpc_serial_read_args)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	uint8_t req_dump[] = { 0x000, 0x000, 0x000, 0x021, 0x080, 0x001, 0x000,
			0x001, 0x000, 0x000, 0x000, 0x004, 0x074, 0x065, 0x073, 0x074,
			0x000, 0x000, 0x000, 0x000, 0x00C, 0x000, 0x001, 0x00B, 0x000,
			0x001, 0x000, 0x000, 0x000, 0x005, 0x068, 0x065, 0x06C, 0x06C,
			0x06F, 0x000, 0x000 };

	buf.put(req_dump, sizeof(req_dump));
	buf.flip();

	sax::thrift_task* ttask = sax::thrift_utility::to_thrift_task(&buf);
	EXPECT_EQ(ttask->fname, "test");
	EXPECT_EQ(ttask->ttype, sax::thrift_task::T_CALL);
	EXPECT_EQ(ttask->seqid, 0);
	EXPECT_EQ(ttask->obj_type, TType::T_TEST_REQ);
	ASSERT_TRUE(ttask->obj != NULL);
	TestReq* req = (TestReq*) ttask->obj;
	EXPECT_EQ(req->req, "hello");

	delete req;

	TestRes* res = new TestRes();
	res->res = "world";

	ttask->ttype = sax::thrift_task::T_REPLY;
	ttask->obj_type = TType::T_TEST_RES;
	ttask->obj = res;

	EXPECT_EQ(buf.remaining(), (size_t)0);
	buf.compact();

	sax::thrift_utility::to_result_buffer(&buf, ttask);

	uint8_t res_dump[] = { 0x000, 0x000, 0x000, 0x021, 0x080, 0x001, 0x000,
			0x002, 0x000, 0x000, 0x000, 0x004, 0x074, 0x065, 0x073, 0x074,
			0x000, 0x000, 0x000, 0x000, 0x00C, 0x000, 0x000, 0x00B, 0x000,
			0x001, 0x000, 0x000, 0x000, 0x005, 0x077, 0x06F, 0x072, 0x06C,
			0x064, 0x000, 0x000 };

	buf.flip();
	EXPECT_EQ(buf.remaining(), sizeof(res_dump));
	uint8_t tmp[100];
	buf.get(tmp, sizeof(res_dump));
	EXPECT_EQ(std::string((char*)res_dump, sizeof(res_dump)), std::string((char*)tmp, sizeof(res_dump)));

	delete ttask;

}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));


    return RUN_ALL_TESTS();
}
