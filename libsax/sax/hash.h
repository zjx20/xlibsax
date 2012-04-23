#ifndef __HASH_H_2008__
#define __HASH_H_2008__

/// @defgroup HASH a set of hashing functions.
/// @author  Qingshan Luo (qluo.cn@gmail.com)
/// @version  0.12
/// @date    Dec. 2008, Jan. 2009, Apr. 2011

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * These two functions are extracted from tr1/functional_hash.h
 * It is said that they are derived from FNV hash:
 * http://isthe.com/chongo/tech/comp/fnv/
 */
static __INLINE__ uint32_t tr1_hash32(const void *s, uint32_t len)
{
	const char *p = (const char *) s;
	uint32_t key = 2166136261U;
	for (; len > 0; --len)
	{
		key ^= (uint32_t)(*p++);
		key *= 16777619U;
	}
	return key;
}

#if defined(UINT64_C)
#define __MAKE_UINT64__(x) UINT64_C(x)
#elif defined(__GNUC__)
#define __MAKE_UINT64__(x) x##ull
#else
#define __MAKE_UINT64__(x) x
#endif

static __INLINE__ uint64_t tr1_hash64(const void *s, uint32_t len)
{
	const char *p = (const char *) s;
	uint64_t key = __MAKE_UINT64__(14695981039346656037);
	for (; len > 0; --len)
	{
		key ^= (uint64_t)(*p++);
		key *= __MAKE_UINT64__(1099511628211);
	}
	return key;
}

#if IS_LITTLE_ENDIAN
#define _PACK_4BYTES_(a) *(uint32_t *)a;
#else
#define _PACK_4BYTES_(a) (a[0]|((uint32_t)a[1]<<8)|((uint32_t)a[2]<<16)|((uint32_t)a[3]<<24));
#endif

/**
 * Some guys argued that murmur-hash is better in both speed 
 * and collision-performance than FNV anyway:
 * http://tech.reus.me/?p=567
 *
 * Here is the code from:
 * http://sites.google.com/site/murmurhash/
 *
 * Perf numbers on Intel Core 2 Duo @ 2.4ghz -
 * - OneAtATime: 354.163715 mb/sec
 * - FNV: 443.668038 mb/sec
 * - SuperFastHash: 985.335173 mb/sec
 * - lookup3: 988.080652 mb/sec
 * - MurmurHash 2.0: 2056.885653 mb/sec
 *
 * And they have a few limitations: (2 has been fixed by _PACK_4BYTES_)
 *  1. It will not work incrementally.
 *  2. Produce different results on little-endian and big-endian machines.
 *
 * Changing the seed value will totally change the output of the hash.
 * If you don't have a preference, use a 'seed' of 0.
 *
 * 'm' and 'r' are mixing constants generated offline.
 * They're not really 'magic', they just happen to work well.
 */
static __INLINE__ uint32_t murmur_hash32(const void *key, uint32_t len)
{
	const uint32_t m = 0x5bd1e995;
	const int r = 24;
	const uint32_t seed = 97;
	uint32_t h = seed ^ len;
	
	// Mix 4 bytes at a time into the hash
	const uint8_t *ptr = (const uint8_t *)key;
	while (len >= 4)
	{
		uint32_t k = _PACK_4BYTES_(ptr);
		k *= m; k ^= k >> r; k *= m; 
		h *= m; h ^= k;
		ptr += 4;
		len -= 4;
	}
	// Handle the last few bytes of the input array
	switch (len)
	{
		case 3: h ^= (uint32_t)ptr[2] << 16;
		case 2: h ^= (uint32_t)ptr[1] << 8;
		case 1: h ^= (uint32_t)ptr[0]; h *= m;
	};
	
	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
}

static __INLINE__ uint64_t murmur_hash64(const void * key, uint32_t len)
{
	const uint32_t m = 0x5bd1e995;
	const int r = 24;
	const uint32_t seed = 97;
	
	uint32_t h1 = seed ^ len;
	uint32_t h2 = 0;
 
	const uint8_t * ptr = (const uint8_t *)key; 
	while (len >= 8)
	{
		uint32_t k = _PACK_4BYTES_(ptr);
		k *= m; k ^= k >> r; k *= m;
		h1 *= m; h1 ^= k;
		len -= 4;
		ptr += 4;
 
		k = _PACK_4BYTES_(ptr);
		k *= m; k ^= k >> r; k *= m;
		h2 *= m; h2 ^= k;
		len -= 4;
		ptr += 4;
	}
 
	if (len >= 4)
	{
		uint32_t k = _PACK_4BYTES_(ptr);
		k *= m; k ^= k >> r; k *= m;
		h1 *= m; h1 ^= k;
		len -= 4;
		ptr += 4;
	}
 
	switch (len)
	{
	case 3: h2 ^= (uint32_t)ptr[2] << 16;
	case 2: h2 ^= (uint32_t)ptr[1] << 8;
	case 1: h2 ^= (uint32_t)ptr[0]; h2 *= m;
	};
 
	h1 ^= h2 >> 18; h1 *= m;
	h2 ^= h1 >> 22; h2 *= m;
	h1 ^= h2 >> 17; h1 *= m;
	h2 ^= h1 >> 19; h2 *= m;
 
	return ((uint64_t)h1 << 32) | h2;
}

/**
 * MySQL: NEW_HASH_FUNCTION
 * 
 * The basis of the hash algorithm was taken from an idea sent by email to the
 * IEEE Posix P1003.2 mailing list from Phong Vo (kpv@research.att.com) and
 * Glenn Fowler (gsf@research.att.com). Landon Curt Noll (chongo@toad.com)
 * later improved on their algorithm.
 *
 * The magic is in the interesting relationship between the special prime
 * 16777619 (2^24 + 403) and 2^32 and 2^8.
 *
 * This hash produces the fewest collisions of any function that we¡¯ve seen so
 * far, and works well on both numbers and strings.
 **/
static __INLINE__ uint32_t hash_ieee(
	uint32_t key, const uint8_t *s, uint32_t len)
{
	while (len--)
		key = (16777619u * key) ^ (uint32_t)*s++;
	return key;
}

/// @brief a primary hashing function for coll_hash,
/// the hashing function with prime number "37"
/// @param start the previous hash value
/// @param s the address of byte stream
/// @param len the length of byte stream.
static __INLINE__ uint32_t hash37(
	uint32_t start, const uint8_t *s, uint32_t len)
{
	uint32_t key = start;
	while (len--) 
		key = 37*key + *s++;
	return key;
}

static __INLINE__ uint32_t hash131(
	uint32_t start, const uint8_t *s, uint32_t len)
{
	uint32_t key = start;
	while (len--) 
		key = 131*key + *s++;
	return key;
}

// hash a variable-length key into a 32-bit value
// Refer: http://burtleburtle.net/bob/c/lookup2.c

#define MIX3_UINT32(a, b, c) \
{ \
	a -= b; a -= c; a ^= (c>>13); \
	b -= c; b -= a; b ^= (a<<8); \
	c -= a; c -= b; c ^= (b>>13); \
	a -= b; a -= c; a ^= (c>>12);  \
	b -= c; b -= a; b ^= (a<<16); \
	c -= a; c -= b; c ^= (b>>5); \
	a -= b; a -= c; a ^= (c>>3);  \
	b -= c; b -= a; b ^= (a<<10); \
	c -= a; c -= b; c ^= (b>>15); \
}

/// @brief generate hashing code by MIX uint32
/// @param c the previous hash value
/// @param s the address of byte stream
/// @param len the length of byte stream.
static __INLINE__ uint32_t mix_hash32(
	uint32_t c, const uint8_t *s, uint32_t len) 
{
	uint32_t  a, b, t = len;
	
	// the golden ratio; an arbitrary value
	b = a = 0x9e3779b9; 

	// (1) handle most of the keys
	while (len >= 12) {
#if (IS_LITTLE_ENDIAN)
		a += *(uint32_t *)(s+0);
		b += *(uint32_t *)(s+4);
		c += *(uint32_t *)(s+8);
#else
		a += (s[0] +((uint32_t)s[1]<<8) +
			((uint32_t)s[2]<<16) +((uint32_t) s[3]<<24));
		b += (s[4] +((uint32_t)s[5]<<8) +
			((uint32_t)s[6]<<16) +((uint32_t) s[7]<<24));
		c += (s[8] +((uint32_t)s[9]<<8) +
			((uint32_t)s[10]<<16)+((uint32_t)s[11]<<24));
#endif
		MIX3_UINT32(a, b, c);
		s += 12;   len -= 12;
	}
	
	// (2) handle the rest 11 bytes, all the case statements 
	// fall through.  NOTE: the first byte of c is reserved 
	// for the stream length. For case 0: nothing left to add.
	c += t;
	switch (len)
	{
		case 11: c+=((uint32_t)s[10]<<24);
		case 10: c+=((uint32_t)s[9]<<16);
		case 9:  c+=((uint32_t)s[8]<<8);
		case 8:  b+=((uint32_t)s[7]<<24);
		case 7:  b+=((uint32_t)s[6]<<16);
		case 6:  b+=((uint32_t)s[5]<<8);
		case 5:  b+=s[4];
		case 4:  a+=((uint32_t)s[3]<<24);
		case 3:  a+=((uint32_t)s[2]<<16);
		case 2:  a+=((uint32_t)s[1]<<8);
		case 1:  a+=s[0];
	}
	MIX3_UINT32(a, b, c);
	return c; // report the result
}



// hash a variable-length key into a 64-bit value
// Refer: http://burtleburtle.net/bob/c/lookup8.c

#define MIX3_UINT64(a,b,c) \
{ \
	a=a-b;  a=a-c;  a=a^(c>>43); \
	b=b-c;  b=b-a;  b=b^(a<<9); \
	c=c-a;  c=c-b;  c=c^(b>>8); \
	a=a-b;  a=a-c;  a=a^(c>>38); \
	b=b-c;  b=b-a;  b=b^(a<<23); \
	c=c-a;  c=c-b;  c=c^(b>>5); \
	a=a-b;  a=a-c;  a=a^(c>>35); \
	b=b-c;  b=b-a;  b=b^(a<<49); \
	c=c-a;  c=c-b;  c=c^(b>>11); \
	a=a-b;  a=a-c;  a=a^(c>>12); \
	b=b-c;  b=b-a;  b=b^(a<<18); \
	c=c-a;  c=c-b;  c=c^(b>>22); \
}

/// @brief generate hashing code by MIX uint64
/// @param c the previous hash value
/// @param s the address of byte stream
/// @param len the length of byte stream.
static __INLINE__ uint64_t mix_hash64(
	uint64_t c, const uint8_t *s, uint32_t len)
{
	uint64_t a, b, t = len;
	
	// the golden ratio; an arbitrary value
	a = b = c;
	c = ((uint64_t)0x9e3779b9UL << 32) | 0x7f4a7c13UL;

	// (1) handle most of the keys
	while (len >= 24) {
#if (IS_LITTLE_ENDIAN)
		a += *(uint64_t *)(s+ 0);
		b += *(uint64_t *)(s+ 8);
		c += *(uint64_t *)(s+16);
#else
		a += (s[0] + ((uint64_t)s[1]<<8) +
			((uint64_t)s[2]<<16)+((uint64_t)s[3]<<24) +
			((uint64_t)s[4]<<32)+((uint64_t)s[5]<<40) + 
			((uint64_t)s[6]<<48)+((uint64_t)s[7]<<56));
		b += (s[8] + ((uint64_t)s[9]<<8) +
			((uint64_t)s[10]<<16)+((uint64_t)s[11]<<24) +
			((uint64_t)s[12]<<32)+((uint64_t)s[13]<<40) + 
			((uint64_t)s[14]<<48)+((uint64_t)s[15]<<56));
		c += (s[16] +((uint64_t)s[17]<<8) + 
			((uint64_t)s[18]<<16)+((uint64_t)s[19]<<24) +
			((uint64_t)s[20]<<32)+((uint64_t)s[21]<<40) +
			((uint64_t)s[22]<<48)+((uint64_t)s[23]<<56));
#endif
		MIX3_UINT64(a, b, c);
		s += 24;   len -= 24;
	}

	// (2) handle the rest 23 bytes, all the case statements 
	// fall through.  NOTE: the first byte of c is reserved 
	// for the stream length. For case 0: nothing left to add.
	c += t;
	switch (len) 
	{
		case 23: c+=((uint64_t)s[22]<<56);
		case 22: c+=((uint64_t)s[21]<<48);
		case 21: c+=((uint64_t)s[20]<<40);
		case 20: c+=((uint64_t)s[19]<<32);
		case 19: c+=((uint64_t)s[18]<<24);
		case 18: c+=((uint64_t)s[17]<<16);
		case 17: c+=((uint64_t)s[16]<<8);
		case 16: b+=((uint64_t)s[15]<<56);
		case 15: b+=((uint64_t)s[14]<<48);
		case 14: b+=((uint64_t)s[13]<<40);
		case 13: b+=((uint64_t)s[12]<<32);
		case 12: b+=((uint64_t)s[11]<<24);
		case 11: b+=((uint64_t)s[10]<<16);
		case 10: b+=((uint64_t)s[9]<<8);
		case 9 : b+=s[8];
		case 8 : a+=((uint64_t)s[7]<<56);
		case 7 : a+=((uint64_t)s[6]<<48);
		case 6 : a+=((uint64_t)s[5]<<40);
		case 5 : a+=((uint64_t)s[4]<<32);
		case 4 : a+=((uint64_t)s[3]<<24);
		case 3 : a+=((uint64_t)s[2]<<16);
		case 2 : a+=((uint64_t)s[1]<<8);
		case 1 : a+=s[0];
	}
	MIX3_UINT64(a, b, c);
	return c; // report the result
}



// hash a variable-length key into two 32-bit values
// Refer: http://burtleburtle.net/bob/c/lookup3.c

#define ROT_UINT(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define MIX_UINT_LOOP(a,b,c) \
{ \
  a -= c;  a ^= ROT_UINT(c, 4);  c += b; \
  b -= a;  b ^= ROT_UINT(a, 6);  a += c; \
  c -= b;  c ^= ROT_UINT(b, 8);  b += a; \
  a -= c;  a ^= ROT_UINT(c,16);  c += b; \
  b -= a;  b ^= ROT_UINT(a,19);  a += c; \
  c -= b;  c ^= ROT_UINT(b, 4);  b += a; \
}

#define MIX_UINT_DONE(a,b,c) \
{ \
  c ^= b; c -= ROT_UINT(b,14); \
  a ^= c; a -= ROT_UINT(c,11); \
  b ^= a; b -= ROT_UINT(a,25); \
  c ^= b; c -= ROT_UINT(b,16); \
  a ^= c; a -= ROT_UINT(c,4);  \
  b ^= a; b -= ROT_UINT(a,14); \
  c ^= b; c -= ROT_UINT(b,24); \
}

/// @brief hash a variable-length key into two 32-bit values
/// it returns two 32-bit hash values instead of just one.
/// *pc is better mixed than *pb, so use *pc first.  If you want
/// a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
/// @param s the address of byte stream
/// @param len the length of byte stream.
/// @param pc IN: primary initval, OUT: primary hash
/// @param pb IN: secondary initval, OUT: secondary hash
static __INLINE__ void hash32_twins( 
  const uint8_t *s, uint32_t len, uint32_t *pc, uint32_t *pb)
{
	uint32_t a, b, c;

	// Set up the internal state
	a = b = c = 0xdeadbeef + ((uint32_t)len) + *pc;
	if (pb) c += *pb;

	// is it a better choice to read 32-bit chunks?
	if (IS_LITTLE_ENDIAN && ((long)s & 0x3)==0)
	{
		const uint32_t *k = (const uint32_t *)s;

		// all but last block: aligned reads
		while (len > 12) {
			a += k[0]; 
			b += k[1]; 
			c += k[2]; 
			MIX_UINT_LOOP(a, b, c); 
			len -= 12; k += 3;
		}
		
		// no mixing for zero length
		s = (const uint8_t *) k;
		switch (len) {
			case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
			case 11: c+=((uint32_t)s[10])<<16;
			case 10: c+=((uint32_t)s[9])<<8;
			case 9 : c+=s[8];
			case 8 : b+=k[1]; a+=k[0]; break;
			case 7 : b+=((uint32_t)s[6])<<16;
			case 6 : b+=((uint32_t)s[5])<<8;
			case 5 : b+=s[4];
			case 4 : a+=k[0]; break;
			case 3 : a+=((uint32_t)s[2])<<16;
			case 2 : a+=((uint32_t)s[1])<<8;
			case 1 : a+=s[0]; break;
			case 0 : *pc=c; if (pb) *pb=b; return;
		}
	} 
	else { // read the key one byte at a time
		while (len > 12)
		{
			a += s[0] + (((uint32_t)s[1])<<8) + 
				(((uint32_t)s[2])<<16) + (((uint32_t)s[3])<<24);
			b += s[4] + (((uint32_t)s[5])<<8) + 
				(((uint32_t)s[6])<<16) + (((uint32_t)s[7])<<24);
			c += s[8] + (((uint32_t)s[9])<<8) +
				(((uint32_t)s[10])<<16)+(((uint32_t)s[11])<<24);
			MIX_UINT_LOOP(a,b,c);
			len -= 12; s += 12;
		}

		// no mixing for zero length
		switch (len) {
			case 12: c+=((uint32_t)s[11])<<24;
			case 11: c+=((uint32_t)s[10])<<16;
			case 10: c+=((uint32_t)s[9])<<8;
			case 9 : c+=s[8];
			case 8 : b+=((uint32_t)s[7])<<24;
			case 7 : b+=((uint32_t)s[6])<<16;
			case 6 : b+=((uint32_t)s[5])<<8;
			case 5 : b+=s[4];
			case 4 : a+=((uint32_t)s[3])<<24;
			case 3 : a+=((uint32_t)s[2])<<16;
			case 2 : a+=((uint32_t)s[1])<<8;
			case 1 : a+=s[0]; break;
			case 0 : *pc=c; if (pb) *pb=b; return;
		}
	}
	MIX_UINT_DONE(a, b, c);
	*pc=c;   if (pb) *pb=b;
}

static __INLINE__ uint32_t hash32_one( 
	uint32_t c, const uint8_t *s, uint32_t len) 
{
	hash32_twins(s, len, &c, 0);
	return c;
}

static __INLINE__ void g0_twin32(const uint8_t *s, uint32_t len, 
	uint32_t *high, uint32_t *low) 
{
	*high = 13, *low = 0;
	hash32_twins(s, len, high, low);
}

static __INLINE__ uint64_t g1_hash64(const uint8_t *s, uint32_t len) 
{
	uint64_t i1 = mix_hash32 (0x00000000, s, len);
	uint64_t i2 = mix_hash32 (0x13572468, s, len);
	return ((i1 << 32) | i2);
}

static __INLINE__ uint64_t g2_hash64(const uint8_t *s, uint32_t len) 
{
	uint64_t c = ((uint64_t)0x7902cac3UL << 32) | 0x9f392d18UL;
	return mix_hash64(c, s, len);
}

static __INLINE__ uint64_t g3_hash64(const uint8_t *s, uint32_t len) 
{
	uint32_t c = 13, b = 0;
	hash32_twins(s, len, &c, &b);
	return (((uint64_t)b)<<32)+c;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif/*__HASH_H_2008__*/
