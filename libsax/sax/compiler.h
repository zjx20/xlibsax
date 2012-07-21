#ifndef COMPILER_H_
#define COMPILER_H_

/// @file compiler.h
/// @brief define features that depend on compiler
/// @author X
/// @date 2012.7.5

#if defined(__GNUC__)
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif//UNUSED_PARAMETER

#endif /* COMPILER_H_ */
