TARGET ?= t_db

CCTOOL = gcc
CXXTOOL = g++
LINKTOOL = g++

INC =  -I. -I../ -I ../../ -I../../../ -I/usr/local/include/

LIB = -L /usr/local/lib/ -lpthread -lsax

DEBUGFLAGS = -g -D_DEBUG -DDEBUG -D_LOG_USE_LOG4CPLUS
CCFLAGS = -Wall
CCFLAGS += $(INC) $(DEBUGFLAGS) -DHAVE_STDINT
CXXFLAGS = $(CCFLAGS) -D__cplusplus 
LINKFLAGS = -lgcc_s $(LIB)

OBJS = ../hash_bucket.o ../hashdb.o ../btree_node.o ../btreedb.o main_test.o

all: $(TARGET)


$(TARGET): $(OBJS)
	@echo Building [$(TARGET)]
	$(LINKTOOL) -o $(TARGET) $(OBJS) $(LINKFLAGS) $(DEBUGFLAGS)

clean: 
	@echo Removing object files
	@rm -f $(OBJS)
	@echo Removing target files
	@rm -f $(TARGET)

%.o: %.c
	@echo Compiling [$<]
	@$(CCTOOL) -o $@  -c $< $(CCFLAGS) $(DEBUGFLAGS)

%.o: %.cpp
	@echo Compiling [$<]
	@$(CXXTOOL) -o $@ -c $< $(CXXFLAGS) $(DEBUGFLAGS)

