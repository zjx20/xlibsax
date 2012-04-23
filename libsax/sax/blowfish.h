#ifndef __BLOWFISH_H_2010__
#define __BLOWFISH_H_2010__

/// @file blowfish.h
/// @brief Header file for blowfish.c
/// @author Qingshan
/// @date 2010.11.17
/// @note ported from the old code by Paul Kocher
///
/// @remark Beware that Blowfish keys repeat such that "ab" = "abab".
/// @remark Endianness conversions are the responsibility of the caller.
///  (To encrypt bytes on a little-endian platforms, you'll probably want
///  to swap bytes around instead of just casting.)

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct {
	uint32_t P[16 + 2];
	uint32_t S[4][256];
}
blowfish_ctx;

// the code does not check key lengths, give key and len!
// the effective length: len=56, 
// if shorter, repeat; if longer, trim
void blowfish_init(blowfish_ctx *ctx, const uint8_t *key, int len);

// plaintext to ciphertext
void blowfish_encrypt(blowfish_ctx *ctx, uint32_t *xl, uint32_t *xr);

// ciphertext to plaintext
void blowfish_decrypt(blowfish_ctx *ctx, uint32_t *xl, uint32_t *xr);


// handle binary stream, ensure size%8==0 first!
int blowfish_enc(blowfish_ctx *ctx, uint8_t *s, int size);
int blowfish_dec(blowfish_ctx *ctx, uint8_t *s, int size);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //__BLOWFISH_H_2010__
