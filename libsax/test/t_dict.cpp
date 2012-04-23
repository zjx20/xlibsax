#include <stdio.h>
#include <string>

#include <sax/c++/dict.h>

int main( int argc, char *argv[] )
{
	typedef sax::dict<std::string, int> test_t;
	
	test_t one;
	one.add("a", 123);
	one.add("b", 223);
	one.add("c", 323);
	
	test_t two;
	two = one;
	int *a = two.get("a");
	printf("%d\n", a?*a:-1);
	int *b = two.get("b");
	printf("%d\n", b?*b:-1);
	int *c = two.get("c");
	printf("%d\n", c?*c:-1);
	int *d = two.get("d");
	printf("%d\n", d?*d:-1);
	return 0;
}
