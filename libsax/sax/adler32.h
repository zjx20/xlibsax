#ifndef __ADLER32_H_2011__
#define __ADLER32_H_2011__

/**
 * @file adler32.h
 * Compute the Adler-32 checksum of a data stream.
 * This is a modified version based on adler32.c 
 * from the zlib library.
 */

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

uint32_t calc_adler32(
	uint32_t adler, const uint8_t *buf, uint32_t len);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* __ADLER32_H_2011__ */
