#include <sax/sha1.h>

/*
Test Vectors (from FIPS PUB 180-1)
"abc"
  A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
  84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
A million repetitions of "a"
  34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
*/

#define BUFSIZE 4096

#include <stdio.h>

void dump(unsigned char hash[20])
{
	int i;
	printf("SHA1=");
	for(i=0;i<20;i++)
		printf("%02x", hash[i]);
	printf("\n");
}

int
main(int argc, char **argv)
{
	SHA1_CTX ctx;
	unsigned char hash[20], buf[BUFSIZE];
	int i;

	for(i=0;i<BUFSIZE;i++)
		buf[i] = i;

	SHA1Init(&ctx);
	for(i=0;i<1000;i++)
		SHA1Update(&ctx, buf, BUFSIZE);
	SHA1Final(&ctx, hash);
	dump(hash);

	SHA1Init(&ctx);
	SHA1Update(&ctx, (const unsigned char*)"abc", 3);
	SHA1Final(&ctx, hash);
	dump(hash);

	SHA1Init(&ctx);
	SHA1Update(&ctx, (const unsigned char*)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56);
	SHA1Final(&ctx, hash);
	dump(hash);
	
	return 0;
}

