#ifndef __OS_TYPES_H_2010__
#define __OS_TYPES_H_2010__

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif//__STDC_LIMIT_MACROS

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS 1
#endif//__STDC_CONSTANT_MACROS

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif//__STDC_FORMAT_MACROS

#ifndef _STDINT_H
#if defined(HAVE_STDINT) || defined(__GNUC__)
#include <stdint.h>
#else
#include "win32/stdint.h"
#endif /* HAVE_STDINT */
#endif /* _STDINT_H */

#ifndef _INTTYPES_H_
#if defined(HAVE_STDINT) || defined(__GNUC__)
#include <inttypes.h>
#else
#include "win32/inttypes.h"
#endif /* HAVE_STDINT */
#endif /* _INTTYPES_H_ */

#ifndef __INLINE__
#if defined(__GNUC__) || defined(__cplusplus) || defined(c_plusplus)
#define __INLINE__ inline
#else
#define __INLINE__ __inline
#endif
#endif//__INLINE__


#ifndef IS_LITTLE_ENDIAN
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
    __BYTE_ORDER == __LITTLE_ENDIAN) || \
    defined(i386) || defined(__i386__) || \
    defined(__i486__) || \
    defined(__i586__) || defined(__i686__) || \
    defined(vax) || \
    defined(MIPSEL) || defined(_MIPSEL) || \
    defined(ns32000) || \
    defined(sun386) || \
	defined(BIT_ZERO_ON_RIGHT) || \
	defined(__alpha__) || defined(__alpha)
#define IS_LITTLE_ENDIAN 1
#else
#define IS_LITTLE_ENDIAN 0
#endif
#endif//IS_LITTLE_ENDIAN


#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif//UNUSED_PARAMETER

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif//ARRAY_SIZE

#ifndef MAKE_UQUAD
#define MAKE_UQUAD(hi, lo) \
	(((uint64_t)((uint32_t)(hi)) << 32) | (uint32_t)(lo))
#endif//MAKE_UQUAD

#endif//__OS_TYPES_H_2010__
