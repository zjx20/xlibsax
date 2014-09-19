#ifndef COMPILER_H_
#define COMPILER_H_

/// @file compiler.h
/// @brief define features that depend on compiler
/// @author X
/// @date 2012.7.5

#define MACRO_TOSTR(x) #x
#define MACRO_ETOSTR(x) MACRO_TOSTR(x) // expand x then to string
#define MACRO_CAT(a, b) a##b
#define MACRO_ECAT(a, b) MACRO_CAT(a, b) // expand a, b then cat

// PRAGMA
#if defined(_MSC_VER)
#define PRAGMA(...) __pragma(__VA_ARGS__)
#else
#define PRAGMA(...) _Pragma(__VA_ARGS__)
#endif // PRAGMA

#if defined( _MSC_VER )
    #define _WITH_SOURCE( msg ) \
        "[" __FILE__ ":" MACRO_ETOSTR(__LINE__) "] " msg
    #define _COMPILE_INFO( info, msg ) \
        PRAGMA( message( info _WITH_SOURCE(msg) ) )
    #define COMPILE_MESSAGE( m )    _COMPILE_INFO("INFO: ", m)
    #define COMPILE_WARNING( w )    _COMPILE_INFO("WARNING: ", w)
    #define COMPILE_ERROR( e )      _COMPILE_INFO("ERROR: ", e); __compile_error
    #define COMPILE_TODO( t )       _COMPILE_INFO("TODO: " t)
#elif defined( __GNUC__ ) || defined( __clang__ )
    #define COMPILE_MESSAGE( m )    PRAGMA(message m)
    #define COMPILE_WARNING( w )    PRAGMA(GCC warning w)
    #define COMPILE_ERROR( e )      PRAGMA(GCC error e)
    #define COMPILE_TODO( t )       PRAGMA(message "TODO: " t)
#endif

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
#if defined(SUPPORT_STATIC_ASSERT)
#define STATIC_ASSERT(expr, msg) static_assert(expr, #msg)
#elif defined(__cplusplus) || defined(c_plusplus)

template <bool flag>
class __static_assert_helper;

template <>
class __static_assert_helper<true> {};

#define STATIC_ASSERT(expr, msg) \
	typedef struct \
	{ __static_assert_helper<(const bool)(expr)> \
	_static_assert_error__##msg[bool(expr)?1:-1]; } \
	MACRO_ECAT(MACRO_ECAT(compile_error_, __LINE__), _##msg)

#else
#define STATIC_ASSERT(expr, msg) \
	typedef struct {char _static_assert_error__##msg[(expr)?1:-1];} \
	MACRO_ECAT(MACRO_ECAT(compile_error_, __LINE__), _##msg)
#endif//STATIC_ASSERT


#if defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
    #define NOINLINE __declspec(noinline)
#else
    COMPILE_WARNING("NOINLINE is unavailable.")
#endif

// thread_local
#if defined(SUPPORT_THREAD_LOCAL)
    #define thread_local thread_local
#elif defined(__GNUC__) || defined(__clang__)
    #define thread_local __thread
#elif defined(_MSC_VER)
    #define thread_local __declspec(thread)
#else
//#warning "thread_local is unavailable."
#endif // thread_local

#endif /* COMPILER_H_ */
