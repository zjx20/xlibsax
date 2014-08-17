CC = gcc
CXX = g++

ifdef WINDIR
INC:=-I.. -I../.. -I/usr/local/include
LIB:=-L.. -lsax -lws2_32
else
INC:=-I.. -I/usr/local/include -I/usr/local/include/thrift
LIB:=-L.. -lsax -lpthread -lgtest #-lthrift

GTEST_DIR=$(shell echo $$GTEST_DIR)
ifneq ($GTEST_DIR, "")
INC+=-I$(GTEST_DIR)/include
LIB+=-L$(GTEST_DIR)/lib/.libs
endif

endif

CCFLAGS = -Wall -DHAVE_STDINT -DTIXML_USE_STL -DTIXML_USE_TICPP

ifdef RELEASE
CCFLAGS += -O3 -DNDEBUG -fstrict-aliasing
else
CCFLAGS += -g -DDEBUG=1 -D_DEBUG -DSPIN_DEBUG=0 -fstrict-aliasing
endif

ifdef PROFILE
CCFLAGS += -pg
endif

all: t_htab.exe t_logger.exe t_timer.exe t_stage.exe t_fsb.exe t_xml.exe t_now.exe \
	t_lru.exe t_lru_b.exe t_dict.exe t_md5.exe t_exp.exe t_cru.exe \
	t_crc.exe t_a32.exe t_pool.exe t_opt.exe t_sha1.exe t_sort.exe \
	t_ptr.exe t_eda_echo_server.exe t_buffer.exe t_tuple.exe \
	#t_buf_transport.exe t_thrift_utility.exe t_thrift_rpc_serial.exe

%.exe: %.cpp ../libsax.a
	$(CXX) $(CCFLAGS) -o $@ $< $(INC) $(LIB)

%: %.cpp ../libsax.a
	$(CXX) $(CCFLAGS) -o $@ $< $(INC) $(LIB)

t_htab.exe:
	$(CC) -Wall -O3 -s -o t_htab.exe t_htab.c $(INC) $(LIB)

t_logger.exe:
	$(CXX) -Wall -O3 -s -o t_logger.exe t_logger.cpp $(INC) $(LIB)

t_nio.exe:
	$(CXX) -Wall -O3 -s -o t_nio.exe t_nio.cpp $(INC) $(LIB)

t_stage.exe:
	$(CXX) -Wall -O3 -s -o t_stage.exe t_stage.cpp $(INC) $(LIB)

t_fsb.exe:
	$(CXX) -Wall -O3 -s -o t_fsb.exe t_fsb.cpp $(INC) $(LIB)

t_xml.exe:
	$(CXX) -Wall -O3 -s -o t_xml.exe t_xml.cpp -DTIXML_USE_TICPP $(INC) $(LIB)

t_now.exe:
	$(CXX) -Wall -O3 -s -o t_now.exe t_now.cpp $(INC) $(LIB)

t_ptr.exe:
	$(CXX) -Wall -O3 -s -o t_ptr.exe t_ptr.cpp $(INC) $(LIB)

t_tuple.exe:
	$(CXX) -Wall -O3 -s -o t_tuple.exe t_tuple.cpp $(INC) $(LIB)

t_lru.exe:
	$(CXX) -Wall -O3 -s -o t_lru.exe t_lru.cpp $(INC) $(LIB)

t_lru_b.exe:
	$(CXX) -Wall -O3 -s -o t_lru_b.exe t_lru_b.cpp $(INC) $(LIB)

t_pool.exe:
	$(CXX) -Wall -O3 -s -o t_pool.exe t_pool.cpp $(INC) $(LIB)

t_dict.exe:
	$(CXX) -Wall -O3 -s -o t_dict.exe t_dict.cpp $(INC) $(LIB)

t_md5.exe:
	$(CXX) -Wall -O3 -s -o t_md5.exe t_md5.cpp $(INC) $(LIB)

t_exp.exe:
	$(CXX) -Wall -O3 -s -o t_exp.exe t_exp.cpp $(INC) $(LIB)

t_cru.exe:
	$(CXX) -Wall -O3 -s -o t_cru.exe t_cru.cpp $(INC) $(LIB)

t_crc.exe:
	$(CXX) -Wall -O3 -s -o t_crc.exe t_crc.cpp $(INC) $(LIB)

t_a32.exe:
	$(CXX) -Wall -O3 -s -o t_a32.exe t_a32.cpp $(INC) $(LIB)

t_opt.exe:
	$(CXX) -Wall -O3 -s -o t_opt.exe t_opt.cpp $(INC) $(LIB)

t_sha1.exe:
	$(CXX) -Wall -O3 -s -o t_sha1.exe t_sha1.cpp $(INC) $(LIB)

t_sort.exe:
	$(CXX) -Wall -O3 -s -o t_sort.exe t_sort.cpp $(INC) $(LIB)
	
t_spin.exe: t_spin.cpp ../libsax.a
	$(CXX) -Wall -g -o t_spin.exe t_spin.cpp $(INC) $(LIB)

t_slab.exe: t_slab.cpp ../libsax.a
	$(CXX) -Wall -g -D_DEBUG -o t_slab.exe t_slab.cpp $(INC) $(LIB)
	
t_slab_benchmark.exe: t_slab_benchmark.cpp ../libsax.a
	$(CXX) -Wall -g -D_DEBUG -o t_slab_benchmark.exe t_slab_benchmark.cpp $(INC) $(LIB)



t_eda_echo_server.exe: t_eda_echo_server.cpp ../libsax.a
	$(CXX) -Wall -g -o t_eda_echo_server.exe t_eda_echo_server.cpp $(INC) $(LIB)

t_log.exe: t_log.cpp ../libsax.a
	$(CXX) -Wall -g -o t_log.exe t_log.cpp ../sax/net/log.cpp $(INC) $(LIB)

t_buffer.exe: t_buffer.cpp ../libsax.a ../sax/net/buffer.h
	$(CXX) -Wall -g -o t_buffer.exe t_buffer.cpp $(INC) $(LIB) -DLOG_OFF

t_buf_transport.exe: test.thrift t_buf_transport.cpp ../libsax.a ../sax/net/t_buffer_transport.h
	rm -rf gen-cpp
	thrift --gen cpp test.thrift
	$(CXX) -Wall -g -o t_buf_transport.exe t_buf_transport.cpp gen-cpp/test_types.cpp $(INC) $(LIB) -DLOG_OFF

t_thrift_utility.exe: test.thrift t_thrift_utility.cpp ../libsax.a
	rm -rf gen-cpp
	../tools/bin/sax_thrift --gen cpp test.thrift
	thrift --gen cpp test.thrift
	$(CXX) -Wall -g -o t_thrift_utility.exe t_thrift_utility.cpp gen-cpp/test_auto_serial.cpp \
	../sax/net/thrift_utility.cpp \
	gen-cpp/Test_auto_rpc_serial.cpp gen-cpp/Test.cpp gen-cpp/test_types.cpp $(INC) $(LIB) -DLOG_OFF

t_thrift_rpc_serial.exe: test.thrift t_thrift_rpc_serial.cpp ../libsax.a
	rm -rf gen-cpp
	thrift --gen cpp test.thrift
	$(CXX) -Wall -g -o t_thrift_rpc_serial.exe t_thrift_rpc_serial.cpp gen-cpp/Test.cpp gen-cpp/test_types.cpp $(INC) $(LIB) -DLOG_OFF

clean:
	@echo Removing target files
	@rm -f *.exe
