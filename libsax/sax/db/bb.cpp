#include <stdio.h>

char vvv[1000];
//char *ccc = vvv;
int sz;
int bb(const void *kk)
{
	const void *key = kk;
	printf("%s\n", key);
	key = vvv;
	printf("%s\n", key);
}

int main()
{
	vvv[0] = 'a';
	vvv[1] = 'b';
	vvv[2] = 0;
	bb("asas");
}
