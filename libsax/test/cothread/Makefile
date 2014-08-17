CC = gcc
CXX = g++

ifdef WINDIR
INC:=-I.. -I../.. -I../../.. -I/usr/local/include
LIB:=-L.. -L../.. -lsax -lws2_32
else
INC:=-I.. -I../.. -I/usr/local/include -I/usr/local/include/thrift
LIB:=-L.. -L../.. -lsax -lpthread -lgtest #-lthrift

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

all: t_cothread

t_cothread: t_cothread.cpp cothread.cpp cothread.h ../../libsax.a
	$(CXX) -Wall -g -o t_cothread t_cothread.cpp cothread.cpp -lpcl $(INC) $(LIB)

clean:
	@echo Removing target files
	@rm -f *.exe
