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

#if defined(__GNUC__)
#define MEMORY_BARRIER(...) __asm__ __volatile__("":::"memory")
#else
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
void __sax_memory_barrier(int, ...);
#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C"
#endif
#define MEMORY_BARRIER(...) __sax_memory_barrier(0, __VA_ARGS__)
#endif//MEMORY_BARRIER

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


#if defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#warning "NOINLINE is unavailable."
#endif

#if defined(__cplusplus) || defined(c_plusplus)
// Use this to get around strict aliasing rules.
// For example, uint64_t i = bitwise_cast<uint64_t>(returns_double());
// The most obvious implementation is to just cast a pointer,
// but that doesn't work.
// For a pretty in-depth explanation of the problem, see
// http://www.cellperformance.com/mike_acton/2006/06/ (...)
// understanding_strict_aliasing.html
template <typename To, typename From>
static inline To bitwise_cast(From from) {
	STATIC_ASSERT(sizeof(From) == sizeof(To), must_have_equal_sizeof);

	// BAD!!!  These are all broken with -O2.
	//return *reinterpret_cast<To*>(&from);  // BAD!!!
	//return *static_cast<To*>(static_cast<void*>(&from));  // BAD!!!
	//return *(To*)(void*)&from;  // BAD!!!

	// Super clean and paritally blessed by section 3.9 of the standard.
	//unsigned char c[sizeof(from)];
	//memcpy(c, &from, sizeof(from));
	//To to;
	//memcpy(&to, c, sizeof(c));
	//return to;

	// Slightly more questionable.
	// Same code emitted by GCC.
	//To to;
	//memcpy(&to, &from, sizeof(from));
	//return to;

	// Technically undefined, but almost universally supported,
	// and the most efficient implementation.
	union {
		From f;
		To t;
	} u;
	u.f = from;
	return u.t;
}
#endif

#endif /* COMPILER_H_ */
