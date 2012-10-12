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

#include "t_buffer_transport.h"
#include "sax/net/buffer.h"
#include "sax/net/linked_buffer.h"

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

	const int RUNTIMES = 1000;

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

	const int RUNTIMES = 1000;

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
		boost::shared_ptr<TBufferTransport<sax::buffer> > transport(
				new TBufferTransport<sax::buffer>());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::buffer buf;

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench1<TBufferTransport<sax::buffer>, sax::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test1: array base buffer use %lu\n", end - start);
	}

	{
		boost::shared_ptr<TBufferTransport<sax::linked_buffer> > transport(
				new TBufferTransport<sax::linked_buffer>());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::linked_buffer buf;

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench1<TBufferTransport<sax::linked_buffer>, sax::linked_buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test1: linked base buffer use %lu\n", end - start);
	}
}

void test2()
{
	{
		boost::shared_ptr<TBufferTransport<sax::buffer> > transport(
				new TBufferTransport<sax::buffer>());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::buffer buf;

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<TBufferTransport<sax::buffer>, sax::buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test2: array base buffer use %lu\n", end - start);
	}

	{
		boost::shared_ptr<TBufferTransport<sax::linked_buffer> > transport(
				new TBufferTransport<sax::linked_buffer>());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::linked_buffer buf;

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench2<TBufferTransport<sax::linked_buffer>, sax::linked_buffer>(protocol.get(), transport.get());
		clock_t end = clock();

		printf("test2: linked base buffer use %lu\n", end - start);
	}
}

void test3()
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

	{
		boost::shared_ptr<TBufferTransport<sax::buffer> > transport(
				new TBufferTransport<sax::buffer>());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::buffer buf;

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench3<TBufferTransport<sax::buffer>, sax::buffer>(
				protocol.get(), transport.get(), rand_op);
		clock_t end = clock();

		printf("test3: array base buffer use %lu\n", end - start);
	}

	{
		boost::shared_ptr<TBufferTransport<sax::linked_buffer> > transport(
				new TBufferTransport<sax::linked_buffer>());
		boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));

		sax::linked_buffer buf;

		transport->set_buffer(&buf, false);

		clock_t start = clock();
		do_bench3<TBufferTransport<sax::linked_buffer>, sax::linked_buffer>(
				protocol.get(), transport.get(), rand_op);
		clock_t end = clock();

		printf("test3: linked base buffer use %lu\n", end - start);
	}
}

int main()
{
	test1();
	test2();
	test3();

	return 0;
}
