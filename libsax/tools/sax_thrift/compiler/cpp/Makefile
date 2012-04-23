
TGTDIR ?= ../../../bin
TARGET ?= sax_thrift

SAXDIR ?= ../../../..
ROOTDIR ?= .

CCTOOL = gcc
CXXTOOL = g++
LINKTOOL = g++
ARTOOL = ar

INC = \
	-I$(ROOTDIR) \
	-I$(ROOTDIR)/src \
	-I$(ROOTDIR)/src/generate \
	-I$(ROOTDIR)/src/parse \
	-I$(SAXDIR) -I/usr/local/inlclude/ \
	-I/usr/local/include/boost/

LIB = -L$(SAXDIR) \
      -lsax \
      -lpthread

#DEBUGFLAGS = -O3
DEBUGFLAGS = -g -D_DEBUG -D_LOG_USE_LOG4CPLUS
CCFLAGS = -Wall
ARFLAGS = r

CCFLAGS += $(DEBUGFLAGS) $(INC)

CXXFLAGS = $(CCFLAGS) -D__cplusplus 

LINKFLAGS = $(LIB)

#sources
SOURCES = $(wildcard ./src/*.cc)
SOURCES += $(wildcard ./src/*.c)
SOURCES += $(wildcard ./src/generate/*.cc)
SOURCES += $(wildcard ./src/parse/*.cc)
SOURCES += thriftl.cc
SOURCES += thrifty.cc

#objects
OBJS := $(patsubst %.cc,%.o,$(SOURCES))
OBJS := $(patsubst %.c,%.o,$(OBJS))

all: $(SOURCES) $(TGTDIR)/$(TARGET)

###############################################################
#### generate lex and yacc source file
###############################################################

LEX= flex
LFLAGS=
AM_LFLAGS=

LEX_OUTPUT_ROOT=

YACC = bison
YFLAGS = -y
AM_YFLAGS = -d

THRIFTL_LL_PATH = ./src/thriftl.ll
THRIFTY_YY_PATH = ./src/thrifty.yy

$(shell echo "%%" > conftest.l)
$(shell echo "a { ECHO; }" >> conftest.l)
$(shell echo "b { REJECT; }" >> conftest.l)
$(shell echo "c { yymore (); }" >> conftest.l)
$(shell echo "d { yyless (1); }" >> conftest.l)
$(shell echo "e { yyless (input () != 0); }" >> conftest.l)
$(shell echo "f { unput (yytext[0]); }" >> conftest.l)
$(shell echo ". { BEGIN INITIAL; }" >> conftest.l)
$(shell echo "%%" >> conftest.l)
$(shell echo "#ifdef YYTEXT_POINTER" >> conftest.l)
$(shell echo "extern char *yytext;" >> conftest.l)
$(shell echo "#endif" >> conftest.l)
$(shell echo "int" >> conftest.l)
$(shell echo "main (void)" >> conftest.l)
$(shell echo "{" >> conftest.l)
$(shell echo "  return ! yylex () + ! yywrap ();" >> conftest.l)
$(shell echo "}" >> conftest.l)
$(shell $(LEX) conftest.l 2>&1)
$(shell rm -f conftest.l)

ifeq ($(shell test -f lex.yy.c; echo $$?), 0)
  LEX_OUTPUT_ROOT := lex.yy
endif

ifeq ($(shell test -f lexyy.c; echo $$?), 0)
  LEX_OUTPUT_ROOT := lexyy;
endif

$(shell rm -f $(LEX_OUTPUT_ROOT).c)

thriftl.cc: $(THRIFTL_LL_PATH)
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $(THRIFTL_LL_PATH)
	sed '/^#/ s|$(LEX_OUTPUT_ROOT)\.c|thriftl.cc|' $(LEX_OUTPUT_ROOT).c >thriftl.cc
	rm -f $(LEX_OUTPUT_ROOT).c

thrifty.cc: $(THRIFTY_YY_PATH)
	$(YACC) $(YFLAGS) $(AM_YFLAGS) $(THRIFTY_YY_PATH)
	if test -f y.tab.h; then \
	  to=`echo "thrifty_H" | sed \
                -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/' \
                -e 's/[^ABCDEFGHIJKLMNOPQRSTUVWXYZ]/_/g'`; \
	  sed -e "/^#/!b" -e "s/Y_TAB_H/$$to/g" -e "s|y\.tab\.h|thrifty.h|" \
            y.tab.h >thrifty.ht; \
	  rm -f y.tab.h; \
	  if cmp -s thrifty.ht thrifty.h; then \
	    rm -f thrifty.ht ;\
	  else \
	    mv thrifty.ht thrifty.h; \
	  fi; \
	fi
	if test -f y.output; then \
	  mv y.output thrifty.output; \
	fi
	sed '/^#/ s|y\.tab\.c|thrifty.cc|' y.tab.c >thrifty.cct && mv thrifty.cct thrifty.cc
	rm -f y.tab.c

###############################################################
#### end
###############################################################

$(TGTDIR)/$(TARGET): $(OBJS)
	@echo Building [$(TARGET)]
	@if ! [ -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi
	$(LINKTOOL) -o $(TGTDIR)/$(TARGET) $(OBJS)  $(LINKFLAGS) $(DEBUGFLAGS)

clean:
	@echo Removing object files
	rm -f $(OBJS)
	@echo Removing target files
	rm -f $(TGTDIR)/$(TARGET)

%.o: %.c
	@echo Compiling [$<]
	$(CCTOOL) -o $@ -c $< $(CCFLAGS)

%.o: %.cpp
	@echo Compiling [$<]
	$(CXXTOOL) -o $@ -c $< $(CXXFLAGS)

%.o: %.cc
	@echo Compiling [$<]
	$(CXXTOOL) -o $@ -c $< $(CXXFLAGS)
	