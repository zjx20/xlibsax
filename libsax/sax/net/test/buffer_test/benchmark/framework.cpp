/*
 * framework.cpp
 *
 *  Created on: 2012-4-6
 *      Author: X
 */

#include <vector>

#include <boost/shared_ptr.hpp>
#include <protocol/TBinaryProtocol.h>

#include "gen-cpp/bench_types.h"

#include "new/buffer.h"
#include "new/t_buffer_transport.h"

#include "new_old/buffer.h"
#include "new_old/t_buffer_transport.h"

#include "old/buffer.h"
#include "old/t_buffer_transport.h"

using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

template <typename TransportType, typename BufferType>
void do_bench1(TBinaryProtocol* protocol, TransportType* transport)
{
	ComplexObj obj;
	obj.a = false;
	obj.b = 85;
	obj.c = 12452;
	obj.d = 789641521;
	obj.e = 4562734614387LL;
	obj.f = 45678648745768.48786412347;
	obj.g = "test string string string string";
	obj.h.push_back(13472189);
	obj.h.push_back(4864231);
	obj.h.push_back(2154556);
	obj.h.push_back(65423241);
	obj.h.push_back(4328644);
	obj.h.push_back(432864);
	obj.h.push_back(789446);
	obj.h.push_back(7894123);
	obj.h.push_back(75315749);
	obj.h.push_back(9125186);

	BufferType* buf = transport->get_buffer();

	const int RUNTIMES = 1000000;

	for(int i=0;i<RUNTIMES;i++) {
		transport->set_buffer(buf, false);
		obj.write(protocol);
		transport->flush();
		buf->flip();
		//printf("remaining: %d\n", buf->remaining());

		transport->set_buffer(buf, true);

		ComplexObj obj2;
		obj2.read(protocol);
		buf->compact();

		if(obj != obj2) {
			fprintf(stderr, "failed to deserial!\n");
		}
	}
}

template <typename TransportType, typename BufferType>
void do_bench2(TBinaryProtocol* protocol, TransportType* transport)
{
	ComplexObj obj;
	obj.a = false;
	obj.b = 85;
	obj.c = 12452;
	obj.d = 789641521;
	obj.e = 4562734614387LL;
	obj.f = 45678648745768.48786412347;
	obj.g = "test string string string string";
	obj.h.push_back(13472189);
	obj.h.push_back(4864231);
	obj.h.push_back(2154556);
	obj.h.push_back(65423241);
	obj.h.push_back(4328644);
	obj.h.push_back(432864);
	obj.h.push_back(789446);
	obj.h.push_back(7894123);
	obj.h.push_back(75315749);
	obj.h.push_back(9125186);

	BufferType* buf = transport->get_buffer();

	const int RUNTIMES = 1000000;

	for(int i=0;i<RUNTIMES;i++) {
		obj.write(protocol);
		transport->flush();
	}

	buf->flip();
	transport->set_buffer(buf, true);

	for(int i=0;i<RUNTIMES;i++) {
		ComplexObj obj2;
		obj2.read(protocol);

		if(obj != obj2) {
			fprintf(stderr, "failed to deserial!\n");
		}
	}
	buf->compact();
}

template <typename TransportType, typename BufferType>
void do_bench3(TBinaryProtocol* protocol, TransportType* transport, std::vector<int>& rand_op)
{
	ComplexObj obj;
	obj.a = false;
	obj.b = 85;
	obj.c = 12452;
	obj.d = 789641521;
	obj.e = 4562734614387LL;
	obj.f = 45678648745768.48786412347;
	obj.g = "test string string string string";
	obj.h.push_back(13472189);
	obj.h.push_back(4864231);
	obj.h.push_back(2154556);
	obj.h.push_back(65423241);
	obj.h.push_back(4328644);
	obj.h.push_back(432864);
	obj.h.push_back(789446);
	obj.h.push_back(7894123);
	obj.h.push_back(75315749);
	obj.h.push_back(9125186);

	BufferType* buf = transport->get_buffer();

	for(size_t i=0;i<rand_op.size();i+=2) {

		transport->set_buffer(buf, false);

		for(int j=rand_op[i];j>0;j--) {
			obj.write(protocol);
			transport->flush();
		}

		buf->flip();
		transport->set_buffer(buf, true);

		for(int j=rand_op[i+1];j>0;j--) {
			ComplexObj obj2;
			obj2.read(protocol);

			if(obj != obj2) {
				fprintf(stderr, "failed to deserial!\n");
			}
		}

		buf->compact();
	}
}

void test1()
{
	{
		boost::shared_ptr<sax::TBufferTransport> transport(new sax::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::simple_block_mempool mempool(512);
		sax::buffer buf(&mempool);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench1<sax::TBufferTransport, sax::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test1: new buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}

	{
		boost::shared_ptr<sax::old::TBufferTransport> transport(new sax::old::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::old::memory_pool pool;
		sax::old::buffer buf(&pool);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench1<sax::old::TBufferTransport, sax::old::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test1: old buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}
}

void test2()
{
	// 512 per block
	{
		boost::shared_ptr<sax::TBufferTransport> transport(new sax::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::simple_block_mempool mempool(512);
		sax::buffer buf(&mempool);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<sax::TBufferTransport, sax::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test2: new buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}

	{
		boost::shared_ptr<sax::old::TBufferTransport> transport(new sax::old::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::old::memory_pool pool;
		sax::old::buffer buf(&pool);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<sax::old::TBufferTransport, sax::old::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test2: old buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}
}

void test3()
{
	// 10240 per block
	{
		boost::shared_ptr<sax::TBufferTransport> transport(new sax::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::simple_block_mempool mempool(10240);
		sax::buffer buf(&mempool);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<sax::TBufferTransport, sax::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test3: new buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}

	{
		boost::shared_ptr<sax::old::TBufferTransport> transport(new sax::old::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::old::memory_pool pool;
		sax::old::buffer buf(&pool, 10240);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<sax::old::TBufferTransport, sax::old::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test3: old buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}
}

void test4()
{
	// 1024 per block, pre-alloc
	{
		boost::shared_ptr<sax::TBufferTransport> transport(new sax::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::simple_block_mempool mempool(1024);

		// 134 * 1000000 / 1024 = 130859
		static void* ptr[130859 + 10];

		for(int i=0;i<130859 + 10;i++) {
			ptr[i] = mempool.allocate();
		}

		for(int i=0;i<130859 + 10;i++) {
			mempool.recycle(ptr[i]);
		}

		sax::buffer buf(&mempool);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<sax::TBufferTransport, sax::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test4: new buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}

	{
		boost::shared_ptr<sax::old::TBufferTransport> transport(new sax::old::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::old::memory_pool pool;
		sax::old::buffer buf(&pool, 134 * 1000000);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<sax::old::TBufferTransport, sax::old::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test4: old buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}
}

void test5()
{
	//srand((unsigned int)time(NULL));
	srand(0);

	const int TOTAL_OP = 2000000;
	const int MAX_OP_ONCE = 1000;

	int total_count = 0;
	int wrote_count = 0;

	std::vector<int> rand_op;

	while(total_count < TOTAL_OP) {
		int tmp = rand() % MAX_OP_ONCE;
		rand_op.push_back(tmp);
		total_count += tmp;
		wrote_count += tmp;

		tmp = rand() % ((std::min(wrote_count / 20, MAX_OP_ONCE)) + 1);
		rand_op.push_back(tmp);
		total_count += tmp;
		wrote_count -= tmp;
	}

	// 512000 per block
	{
		boost::shared_ptr<sax::TBufferTransport> transport(new sax::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::simple_block_mempool mempool(512000);
		sax::buffer buf(&mempool);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench3<sax::TBufferTransport, sax::buffer>(protocol.get(), transport.get(), rand_op);
		clock_t end = clock();

		printf("test5: new buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}

	{
		boost::shared_ptr<sax::old::TBufferTransport> transport(new sax::old::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::old::memory_pool pool;
		sax::old::buffer buf(&pool, 512000);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench3<sax::old::TBufferTransport, sax::old::buffer>(protocol.get(), transport.get(), rand_op);
		clock_t end = clock();

		printf("test5: old buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}
}

void test6()
{
	// 1024 per block, pre-alloc
	//if(0)
	{
		boost::shared_ptr<sax::TBufferTransport> transport(new sax::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::simple_block_mempool mempool(1024);

		// 134 * 1000000 / 1024 = 130859
		static void* ptr[130859 + 10];

		for(int i=0;i<130859 + 10;i++) {
			ptr[i] = mempool.allocate();
		}

		for(int i=0;i<130859 + 10;i++) {
			mempool.recycle(ptr[i]);
		}

		sax::buffer buf(&mempool);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<sax::TBufferTransport, sax::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test6: new buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}

	//if(0)
	{
		boost::shared_ptr<sax::newold::TBufferTransport> transport(new sax::newold::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::newold::simple_block_mempool mempool(1024);

		// 134 * 1000000 / 1024 = 130859
		static void* ptr[130859 + 10];

		for(int i=0;i<130859 + 10;i++) {
			ptr[i] = mempool.allocate();
		}

		for(int i=0;i<130859 + 10;i++) {
			mempool.recycle(ptr[i]);
		}

		sax::newold::buffer buf(&mempool);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<sax::newold::TBufferTransport, sax::newold::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test6: new buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}

	//if(0)
	{
		boost::shared_ptr<sax::old::TBufferTransport> transport(new sax::old::TBufferTransport());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::old::memory_pool pool;
		sax::old::buffer buf(&pool, 134 * 1000000);

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<sax::old::TBufferTransport, sax::old::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test6: old buffer use %lfs\n", (end - start) * 1.0 / CLOCKS_PER_SEC);
	}
}

int main()
{
	//test1();
	//test2();
	//test3();
	//test4();
	//test5();
	test6();

	return 0;
}
