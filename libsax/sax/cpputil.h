#ifndef CPPUTIL_H_
#define CPPUTIL_H_

/// @file cpputil.h
/// @brief cpp template utility
/// @author X
/// @date 2014.7.30

#include "compiler.h"

#if defined(__cplusplus) || defined(c_plusplus)

namespace sax {

// Use this to get around strict aliasing rules.
// For example, uint64_t i = bitwise_cast<uint64_t>(returns_double());
// The most obvious implementation is to just cast a pointer,
// but that doesn't work.
// For a pretty in-depth explanation of the problem, see
// http://www.cellperformance.com/mike_acton/2006/06/ (...)
// understanding_strict_aliasing.html
template <typename To, typename From>
static inline To bitwise_cast(From from) {
    STATIC_ASSERT(sizeof(From) == sizeof(To), object_size_must_be_equal);

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

//
// usage:
//
//   class A : private virtual nonderivable<A>
//   {
//       ...
//   };
//
template <class C>
class nonderivable
{
    template <typename T> struct type_traits { typedef T type; };
    friend class type_traits<C>::type;
    nonderivable() {}
    virtual ~nonderivable() {}
};

} // namespace sax

#endif // defined(__cplusplus) || defined(c_plusplus)
