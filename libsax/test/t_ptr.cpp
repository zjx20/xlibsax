#include <stdio.h>

#include <sax/c++/smartptr.h>

static void release(int x)
{
	printf("simulate release(%d)...\n", x);
}

void test1()
{
	printf("---testing sax::cleanup<TWIN>: \n");
	int x = 100;
	sax::cleanup abcd(release, x);
}

struct TWIN {
	TWIN() {printf("construct.\n");}
	TWIN(int _a, int _b):a(_a),b(_b) {printf("construct2.\n");}
	~TWIN() {printf("deconstruct.\n");}
	int a, b;
};

void test2()
{
	printf("---testing sax::autoref<TWIN>: \n");
	sax::autodel<TWIN> m(new TWIN(100,200));
	printf( "%d, %d!\n", m->a, m->b);
	
	m = new TWIN;
	m->a = 1000;
	m->b = 2000;
	printf( "%d, %d!\n", m->a, m->b);
}

void test3()
{
	printf("---testing sax::autoref<TWIN>: \n");
	sax::autoref<TWIN> m;
	m->a = 100;
	m->b = 200;
	printf( "%d, %d!\n", m->a, m->b);
	
	m = new TWIN;
	m->a = 1000;
	m->b = 2000;
	printf( "%d, %d!\n", m->a, m->b);
}

void test4()
{
	printf("---testing sax::counted<TWIN>: \n");
	
	sax::counted<TWIN> x(new TWIN);
	*x = TWIN(100, 10000);
	printf( "%d, %d!\n", x->a, x->b);
	
	TWIN &t = x;
	t.a = 200;
	printf( "%d, %d!\n", x->a, x->b);
	
	TWIN *p = (TWIN *)x;
	p->a = 300;
	printf( "%d, %d!\n", x->a, x->b);
}

int main( int argc, char *argv[] )
{
	test1();
	test2();
	test3();
	test4();
	return 0;
}

