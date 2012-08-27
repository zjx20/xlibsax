
INC := -I../../../../../
LIB := -L../../../../../ -lgtest -lpthread
#FLAG := -g -Wall # -fprofile-arcs -ftest-coverage
FLAG := -O3 -Wall# -fno-strict-aliasing

# 

unittest: t_buffer.cpp ../../../buffer.h
	g++ -o $@ t_buffer.cpp ../../../buffer.cpp $(INC) $(LIB) $(FLAG)
	

clean:
	rm -f unittest
