#------------------------------------------------------------------------------
PROJ_ROOT =./
#------------------------------------------------------------------------------
# include
INC_COMM        = -I$(PROJ_ROOT) -I/usr/local/include/thrift
INC_EXT_COMM    = -I../../.. -I../.. -I..
INC             = $(INC_COMM) $(INC_EXT_COMM)
#------------------------------------------------------------------------------
# lib

SYS_LIB         = 
EXT_LIB         = -L../../.. -lsax -lpthread -lthrift
LIB             = $(SYS_LIB) $(EXT_LIB)
#------------------------------------------------------------------------------
 
C_FLAGS = -g -Wall #-O3 -fomit-frame-pointer
LD_FLAGS        =
CXX             = g++ 
RANLIB          = ranlib
AR              = ar

SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))
OK = \\e[1m\\e[32m OK \\e[m
TARGET = server client
FAILURE = \\e[1m\\e[31m FAILURE \\e[m
all : $(TARGET)

server : thrift
	$(CXX) $(C_FLAGS) -o server t_server.cpp ../connection_mgr.cpp \
	../netutil.cpp ../task_streamer.cpp ../tcp_acceptor.cpp ../tcp_connection.cpp \
	../transport.cpp ../thrift_utility.cpp ../thrift_task_streamer.cpp ../log.cpp \
	gen-cpp/test_types.cpp gen-cpp/test_auto_serial.cpp \
	gen-cpp/Test_auto_rpc_serial.cpp gen-cpp/Test.cpp \
	$(INC) $(LIB) -DLOG_OFF

client : thrift
	$(CXX) $(C_FLAGS) -o client t_client.cpp ../connection_mgr.cpp \
	../netutil.cpp ../task_streamer.cpp ../tcp_acceptor.cpp ../tcp_connection.cpp \
	../transport.cpp ../thrift_utility.cpp ../thrift_task_streamer.cpp ../log.cpp \
	gen-cpp/test_types.cpp gen-cpp/test_auto_serial.cpp \
	gen-cpp/Test_auto_rpc_serial.cpp gen-cpp/Test.cpp \
	$(INC) $(LIB) -DLOG_OFF

thrift: test.thrift
	rm -rf gen-cpp
	thrift --gen cpp --gen py test.thrift
	../../../tools/bin/sax_thrift --gen cpp test.thrift

clean:
	@rm -f *.o 
	@rm -f ./$(TARGET)
