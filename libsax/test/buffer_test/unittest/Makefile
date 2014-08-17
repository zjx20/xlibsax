CC = gcc
CXX = g++

SAXDIR = ../../..

ifdef WINDIR
INC:=-I$(SAXDIR) -I/usr/local/include
LIB:=-L$(SAXDIR) -lsax -lws2_32
else
INC:=-I$(SAXDIR) -I/usr/local/include -I/usr/local/include/thrift
LIB:=-L$(SAXDIR) -L/usr/local/lib/gtest -lsax -lpthread -lgtest -lthrift
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

TARGET = t_buffer t_linked_buffer

all: $(TARGET)
#	for i in `find -iname '*.cpp' -printf '%f\n' | \
#			awk '{print substr($$0, 0, length($$0)-3)}'`; do \
#		echo $$i; \
#		make $$i; \
#	done

%: %.cpp ../../../libsax.a
	$(CXX) $(CCFLAGS) -o $@ $< $(INC) $(LIB)

clean:
	@echo Removing target files
	@rm $(TARGET) -f
