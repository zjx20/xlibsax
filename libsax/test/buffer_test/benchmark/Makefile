
SAXDIR = ../../../

INC := -I$(SAXDIR) -I/usr/local/include/thrift
LIB := -L$(SAXDIR) -lpthread -lsax

ifeq '$(shell echo $${OSTYPE})' 'cygwin'
  INC += -I/cygdrive/e/lib/boost_1_48_0 -I/cygdrive/e/lib/thrift-0.7.0/thrift-0.7.0/lib/cpp/src
else
  $(warning "hello")
endif

#FLAG := -g -D_DEBUG -pg
#FLAG := -O0 -DNDEBUG -pg
#FLAG := -O3 -DNDEBUG -pg
FLAG := -g -DNDEBUG

#FLAG := -O3 -DNDEBUG

#FLAG := -O3 -DNDEBUG -lprofiler

bench: framework.cpp
	g++ -o $@ framework.cpp gen-cpp/bench_types.cpp $(FLAG) $(INC) $(LIB)
	

clean:
	rm -f bench
