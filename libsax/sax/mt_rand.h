#ifndef __MT_RAND_H_2010__
#define __MT_RAND_H_2010__

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define MT_RAND_MAX (0x7fFFffFF)

typedef struct mt_str_t mt_str_t;

mt_str_t *mt_seed(uint32_t seed);
void mt_kill(mt_str_t *mt);
mt_str_t *mt_seed_ex(const uint32_t *seed, int len);

int32_t mt_rand(mt_str_t *mt);

uint32_t mt_rand_u32(mt_str_t *mt);
double   mt_rand_r53(mt_str_t *mt);

double mt_rand_r1(mt_str_t *mt); //[0,1]
double mt_rand_r2(mt_str_t *mt); //[0,1)
double mt_rand_r3(mt_str_t *mt); //(0,1)

int32_t mt_range(mt_str_t *mt, int32_t tmin, int32_t tmax);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

