#include <stdio.h>
#include <string>

#include <sax/c++/crumb.h>

int main( int argc, char *argv[] )
{
	typedef int TYPE;
	sax::crumb<char, TYPE> x;
	x.init(2, 100);
	
	x.add(1, 100);
	x.add(2, 200);
	x.add(3, 300);
	
	x.purge();
	printf("size()=%u\n", x.size());
	
	getchar();
	
	x.purge();
	printf("size()=%u\n", x.size());
	
	TYPE z;
	if (x.sub(1, &z)) printf("got: %d\n", z);
	
	x.purge();
	printf("size()=%d\n", x.size());
	
	getchar();
	
	x.purge();
	printf("size()=%d\n", x.size());

	return 0;
}
