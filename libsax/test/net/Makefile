CC = gcc
CXX = g++

ifdef WINDIR
INC:=-I.. -I../.. -I../../..
LIB:=-L.. -L../.. -lsax -lws2_32
else
INC:=-I../.. -I/usr/local/include -I/usr/local/include/thrift
LIB:=-L../.. -lsax -lpthread #-lgtest -lthrift
endif

CCFLAGS = -Wall -DHAVE_STDINT -DTIXML_USE_STL -DTIXML_USE_TICPP #-DLOG_OFF

ifdef RELEASE
CCFLAGS += -O3 -DNDEBUG -fstrict-aliasing
else
CCFLAGS += -g -DDEBUG=1 -D_DEBUG -DSPIN_DEBUG=0 -fstrict-aliasing
endif

ifdef PROFILE
CCFLAGS += -pg
endif

%: %.cpp ../../libsax.a
	$(CXX) $(CCFLAGS) -o $@ $< $(INC) $(LIB)

clean:
	@echo Removing target files
	@rm *.exe -f
