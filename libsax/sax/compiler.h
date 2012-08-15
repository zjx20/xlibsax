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

//STATIC_ASSERT
#if SUPPORT_STATIC_ASSERT
#define STATIC_ASSERT(expr, msg) static_assert(expr, #msg)
#elif defined(__cplusplus) || defined(c_plusplus)

template <bool flag>
class __static_assert_helper;

template <>
class __static_assert_helper<true> {};

#define STATIC_ASSERT(expr, msg) \
	typedef struct \
	{ __static_assert_helper<bool(expr)> _static_assert_error__##msg[bool(expr)?1:-1]; } \
	compile_error##__LINE__##msg;

#else
#define STATIC_ASSERT(expr, msg) typedef struct {char _static_assert_error__##msg[(expr)?1:-1];} compile_error##__LINE__##msg;
#endif//STATIC_ASSERT

#if defined(__cplusplus) || defined(c_plusplus)
#endif

#endif /* COMPILER_H_ */
