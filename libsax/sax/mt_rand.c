// Mersenne Twister random number generator
// Based on code by Makoto Matsumoto, Takuji Nishimura, and
// Shawn Cokus; Richard J. Wagner  v1.1  28 September 2009

// The Mersenne Twister is an algorithm for generating random 
// numbers. It was designed with consideration of the flaws in
// various other generators.
// The period, 2^19937-1, and the order of equidistribution, 
// 623 dimensions, are far greater. The generator is also fast; 
// it avoids multiplication and division, and it benefits from 
// caches and pipelines.  For more information see
// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html

// Reference
// M. Matsumoto and T. Nishimura, "Mersenne Twister: A 623-
// Dimensionally Equidistributed Uniform Pseudo-Random Number 
// Generator", ACM Transactions on Modeling and Computer 
// Simulation, Vol. 8, No. 1, January 1998, pp 3-30.

#include <stdlib.h>
#include <memory.h>

#include "mt_rand.h"

#define MT_N    (624)  // length of state vector
#define MT_M    (397)  // a period parameter

// mask all but highest bit
#define hiBit(u)  ((u) & 0x80000000U)

// mask all but lowest bit
#define loBit(u)  ((u) & 0x00000001U)

// mask the rest bits except highest
#define loBits(u) ((u) & 0x7FFFFFFFU)

// move hi bit of u to hi bit of v
#define mixBits(u, v) (hiBit(u)|loBits(v))

// perform twist operations: two versions
#if defined(USE_PHP_STYLE)
#define twist(m,u,v) (m ^ (mixBits(u,v)>>1) ^ \
	((uint32_t)(-(int32_t)(loBit(u))) & 0x9908b0dfU))
#else // Cokus
#define twist(m,u,v) (m ^ (mixBits(u,v)>>1) ^ \
	(loBit(v) ? 0x9908b0dfU : 0UL))
#endif // USE_PHP_STYLE


struct mt_str_t {
	uint32_t   state[MT_N+1];// +1 extra to not violate ANSI C
	int        left;
	uint32_t   *next; // ++ many times before reloading
};


// Initialize generator state with seed
// See Knuth TAOCP Vol 2, 3rd Ed, p.106 for multiplier.
// In previous versions, most significant bits (MSBs) 
// of the seed affect only MSBs of the state array.  
// Modified 9 Jan 2002 by Makoto Matsumoto.
static void init_state(uint32_t seed, uint32_t *state)
{
	register uint32_t *s = state;
	register uint32_t *r = state;
	register int i = 1;

	*s++ = seed & 0xffffffffU;
	for( ; i < MT_N; ++i, r++) {
		*s++ = ( 1812433253U * ( *r ^ (*r >> 30) ) + i ) & 0xffffffffU;
	}
}

// Seed the generator with an array of uint32_t elements
// There are 2^19937-1 possible initial states. This function 
// allows all of those to be accessed by providing at least 
// 19937 bits (with a default seed length of MT_N=624 uint32). 
// Any bits above the lower 32 in each element are discarded.
static void init_state_ex(const uint32_t *seed, int len, uint32_t *state)
{
	uint32_t j = 0;
	int i = 1;
	int k = (MT_N > len ? MT_N : len);
	init_state(19650218U, state);
	
	for ( ; k; --k )
	{
		state[i] ^= (state[i-1] ^ (state[i-1] >> 30)) * 1664525U;
		state[i] += (seed[j] + j);
		++i;  ++j;
		if (i >= MT_N) { state[0] = state[MT_N-1];  i = 1; }
		if (j >= len) j = 0;
	}
	
	for (k = MT_N-1; k; --k )
	{
		state[i] ^= (state[i-1] ^ (state[i-1] >> 30)) * 1566083941U;
		state[i] -= i;
		++i;
		if (i >= MT_N) { state[0] = state[MT_N-1];  i = 1; }
	}
	
	// MSB is 1, assuring non-zero initial array
	state[0] = 0x80000000U;
}

// Generate MT_N new values in state
// Made clearer and faster by Matthew Bellew
static void renew_state(uint32_t *state)
{
	register uint32_t *p = state;
	register int i;

	for (i = MT_N - MT_M; i--; ++p)
		*p = twist(p[MT_M], p[0], p[1]);
	
	for (i = MT_M; --i; ++p)
		*p = twist(p[MT_M-MT_N], p[0], p[1]);
	
	*p = twist(p[MT_M-MT_N], p[0], state[0]);
}


// Pull a 32-bit integer from the generator state.
// Every other access function simply transforms 
// the numbers extracted here.
static uint32_t exec_rand(mt_str_t *mt)
{
	register uint32_t s1;
	if (mt->left == 0) 
	{
		renew_state(mt->state);
		mt->left = MT_N;
		mt->next = mt->state;
	}
	
	mt->left--; 
	s1 = *mt->next++;
	
	s1 ^= (s1 >> 11);
	s1 ^= (s1 <<  7) & 0x9d2c5680U;
	s1 ^= (s1 << 15) & 0xefc60000U;
	return ( s1 ^ (s1 >> 18) );
}


// Seed the generator with a simple uint32_t
mt_str_t *mt_seed(uint32_t seed)
{
	mt_str_t *mt = (mt_str_t *) malloc(sizeof(mt_str_t));
	if (mt) {
		if (seed == 0) seed = 5489U;
		init_state(seed, mt->state);
		mt->left = 0;
		mt->next = NULL;
	}
	return mt;
}

void mt_kill(mt_str_t *mt)
{
	if (mt) free(mt);
}

// Seed the generator with an array of uint32_t
mt_str_t *mt_seed_ex(const uint32_t *seed, int len)
{
	mt_str_t *mt = (mt_str_t *) malloc(sizeof(mt_str_t));
	if (mt) {
		init_state_ex(seed, len, mt->state);
		mt->left = 0;
		mt->next = NULL;
	}
	return mt;
}


int32_t mt_rand(mt_str_t *mt)
{
	return (int32_t)(exec_rand(mt)>>1);
}

uint32_t mt_rand_u32(mt_str_t *mt)
{
	return exec_rand(mt);
}

// Access to 53-bit random floating numbers 
// (capacity of IEEE double precision)
double mt_rand_r53(mt_str_t *mt)
{
	uint32_t a = exec_rand(mt) >> 5;
	uint32_t b = exec_rand(mt) >> 6;
	return ( a * 67108864.0 + b ) * (1.0/9007199254740992.0);
}

// generates a random number on [0,1]-real-interval 
double mt_rand_r1(mt_str_t *mt)
{
    return (double)exec_rand(mt) * (1.0/4294967295.0); 
}

// generates a random number on [0,1)-real-interval 
double mt_rand_r2(mt_str_t *mt)
{
    return (double)exec_rand(mt) * (1.0/4294967296.0); 
}

// generates a random number on (0,1)-real-interval 
double mt_rand_r3(mt_str_t *mt)
{
    return ((double)exec_rand(mt) + 0.5) * (1.0/4294967296.0); 
}

int32_t mt_range(mt_str_t *mt, int32_t tmin, int32_t tmax)
{
	double x = (tmax-tmin+1) * mt_rand_r2(mt);
	return (tmin + (int32_t)x);
}
