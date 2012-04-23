#include <stdio.h>
#include <string>
#include <sax/c++/pools.h>

int main( int argc, char *argv[] )
{
	sax::spool<int> m;
	m.init2(NULL, 8);
	int *p1 = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), p1);
	int *p2 = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), p2);
	int *p3 = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), p3);
	int *p4 = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), p4);
	int *p5 = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), p5);
	int *p6 = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), p6);
	int *p7 = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), p7);
	int *p8 = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), p8);
	int *p9 = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), p9);
	m.free_obj(p1); printf("%d,%d\n", m.used(), m.capacity());
	m.free_obj(p2); printf("%d,%d\n", m.used(), m.capacity());
	m.free_obj(p6); printf("%d,%d\n", m.used(), m.capacity());
	m.free_obj(p7); printf("%d,%d\n", m.used(), m.capacity());
	m.free_obj(p7); printf("%d,%d\n", m.used(), m.capacity());
	int *px = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), px);
	int *py = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), py);
	int *pz = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), pz);
	int *pu = m.alloc_obj(); printf("%d,%d: %p\n", m.used(), m.capacity(), pu);
	
	sax::spool<std::string> s;
	s.init2(NULL, 4);
	std::string *s1 = s.alloc_obj("dog"); printf("%d,%d: %s\n", s.used(), s.capacity(), s1->c_str());
	std::string *s2 = s.alloc_obj("cat"); printf("%d,%d: %s\n", s.used(), s.capacity(), s2->c_str());
	s.free_obj(s1);
	s.free_obj(s2);
	
	printf("g_shm_unit()=%d\n", g_shm_unit());
	
	sax::mpool x;
	x.init(NULL, 256*1024);
	int *x1 = x.alloc_obj<int>();
	int64_t *x2 = x.alloc_obj<int64_t>();
	double *x3 = x.alloc_obj<double>();
	printf("%p, %p, %p\n", x1, x2, x3);
	return 0;
}
