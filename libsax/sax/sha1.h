#ifndef __SHA1_H_2011__
#define __SHA1_H_2011__

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, const unsigned char* data, uint32_t len);
void SHA1Final(SHA1_CTX* context, unsigned char digest[20]);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //__SHA1_H_2011__

