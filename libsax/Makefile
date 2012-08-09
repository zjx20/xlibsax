TARGET = libsax.a

OBJS = $(patsubst %.c,%.o,$(wildcard sax/*.c))
OBJS+= $(patsubst %.cpp,%.o,$(wildcard sax/*.cpp))
OBJS+= $(patsubst %.cpp,%.o,$(wildcard sax/stage/*.cpp))
OBJS+= $(patsubst %.cpp,%.o,$(wildcard sax/xml/*.cpp))

HDRS = $(wildcard stx/*.h)
HDRS+= $(wildcard sax/*.h)
HDRS+= $(wildcard sax/win32/*.h)
HDRS+= $(wildcard sax/c++/*.h)
HDRS+= $(wildcard sax/stage/*.h)
HDRS+= $(wildcard sax/xml/*.h)

INC = -I. 

prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

CC = gcc
CXX = g++
LINK = g++
AR = ar
RANLIB = ranlib

ARFLAGS = rc
CCFLAGS = -Wall -DHAVE_STDINT -DTIXML_USE_STL -DTIXML_USE_TICPP
LIB = 

ifdef DEBUG
CCFLAGS += -g -DDEBUG=1 -D_DEBUG -DSPIN_DEBUG=0 -fstrict-aliasing
else
CCFLAGS += -O3 -DNDEBUG -fstrict-aliasing
endif

ifdef PROFILE
CCFLAGS += -pg
endif

ifdef WINDIR
LIB = -lws2_32
else
CCFLAGS += -fPIC -DHAVE_EPOLL
endif

CXXFLAGS = $(CCFLAGS) -D__cplusplus 


all: $(TARGET)

$(TARGET): $(OBJS)
	@echo Achiving [$(TARGET)]
	@$(AR) $(ARFLAGS) $(TARGET) $(OBJS)
	@$(RANLIB) $(TARGET)

shared: $(OBJS)
	@echo Achiving shared library
	@$(CXX) -shared -o $(patsubst %.a,%.so,$(TARGET)) $(OBJS) $(LIB)

clean: 
	@echo Removing object files
	@rm -f $(OBJS)
	@echo Removing target files
	@rm -f $(TARGET)

install: $(TARGET)
	@list='$(HDRS)'; for p in $$list; do \
	  echo Installing header [$$p]; \
	  f=$(includedir)/$$p; \
	  d=`dirname $$f`; \
	  test -d "$$d" || mkdir -p "$$d"; \
	  if [ -f "$$p" ]; then \
	    cp -u -p "$$p" "$$f"; \
	  else \
	    cp -u -p -r "$$p" "$$d"; \
	  fi \
	done
	@echo Installing library [$(TARGET)]
	@mkdir -p $(libdir)
	@cp -u -p $(TARGET) $(libdir)/

uninstall:
	@list='$(HDRS)'; for p in $$list; do \
	  echo Removing header [$$p]; \
	  rm -rf "$(includedir)/$$p"; \
	done
	@echo Removing empty directories
	@/usr/bin/find '$(includedir)' -maxdepth 2 -type d -empty -print | xargs rm -rf
	@/usr/bin/find '$(includedir)' -maxdepth 1 -type d -empty -print | xargs rm -rf
	@echo Removing library [$(TARGET)]
	@rm -f $(libdir)/$(TARGET)

%.o: %.c
	@echo Compiling [$<]
	@$(CC) -o $@  -c $< $(CCFLAGS) $(INC)

%.o: %.cpp
	@echo Compiling [$<]
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $(INC)
