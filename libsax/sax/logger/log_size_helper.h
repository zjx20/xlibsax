/*
 * log_size_helper.h
 *
 *  Created on: 2012-8-22
 *      Author: x
 */

#ifndef _SAX_LOG_SIZE_HELPER_H_
#define _SAX_LOG_SIZE_HELPER_H_

#include <cfloat>
#include <cstring>
#include <string>
#include "sax/os_types.h"
#include "sax/compiler.h"

namespace sax {
namespace logger {

namespace util {

template <int32_t NUMBER, int32_t BASE = 10>
struct number_digits_traits_helper;

template <int32_t BASE>
struct number_digits_traits_helper<0, BASE> { enum { VALUE = 0 }; };

template <int32_t NUMBER, int32_t BASE>
struct number_digits_traits_helper
{
	enum
	{
		VALUE = number_digits_traits_helper<NUMBER / BASE, BASE>::VALUE + 1
	};
};

template <int32_t NUMBER, int32_t BASE = 10>
struct number_digits_traits;

template <int32_t BASE>
struct number_digits_traits<0, BASE> { enum { VALUE = 1 }; };

template <int32_t NUMBER, int32_t BASE>
struct number_digits_traits
{
	enum
	{
		VALUE = number_digits_traits_helper<NUMBER, BASE>::VALUE +
					((NUMBER < 0) ? 1 : 0)
	};
};

template <int32_t A, int32_t B>
struct max { enum { VALUE = (A > B ? A : B) }; };

} // util namespace

#define DIGIT_LEN_MAX(a, b) \
	util::max<util::number_digits_traits<int32_t(a), 10>::VALUE, \
	util::number_digits_traits<int32_t(b), 10>::VALUE>::VALUE

struct estimated_size
{
	enum
	{
		BOOL	= 1,
		CHAR	= 1,
		UINT8	= 3,
		INT16	= 5 + 1,	// 5 digits + 1 sign symbol in longest case
		UINT16	= 5,
		INT32	= 10 + 1,	// 10 digits + 1 sign symbol in longest case
		UINT32	= 10,
		INT64	= 19 + 1,	// 19 digits + 1 sign symbol in longest case
		UINT64	= 20,
		POINTER	= sizeof(void*) * 2 + 2,	// treat as unsigned int, in hex and have '0x' prefix

		// floating point number is present in scientific notation,
		// 9 characters for mantissa, including sign symbol, eg: "-0.123456";
		// 1 character for exponent symbol, eg: "e";
		// 3~5 characters for exponent, including sign symbol, eg: "-38"
		FLOAT		= 9 + 1 + DIGIT_LEN_MAX(FLT_MIN_10_EXP, FLT_MAX_10_EXP),
		DOUBLE		= 9 + 1 + DIGIT_LEN_MAX(DBL_MIN_10_EXP, DBL_MAX_10_EXP),
		LONGDOUBLE	= 9 + 1 + DIGIT_LEN_MAX(LDBL_MIN_10_EXP, LDBL_MAX_10_EXP),
	};
};

#undef DIGIT_LEN_MAX

template <size_t SIZE>
struct static_log_size_helper {};

template <size_t SIZE>
struct dynamic_log_size_helper
{
	dynamic_log_size_helper() : size(0) {}
	dynamic_log_size_helper(size_t s) : size(s) {}
	size_t size;
};

#define DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(type_size, type) \
	template <size_t SIZE> static_log_size_helper<SIZE + type_size> \
	operator << (const static_log_size_helper<SIZE>&, const type&) \
	{ return static_log_size_helper<SIZE + type_size>(); }

DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::BOOL, bool);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::CHAR, char);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::UINT8, uint8_t);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::INT16, int16_t);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::UINT16, uint16_t);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::INT32, int32_t);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::UINT32, uint32_t);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::INT64, int64_t);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::UINT64, uint64_t);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::POINTER, void*);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::FLOAT, float);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::DOUBLE, double);
DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::LONGDOUBLE, long double);

#undef DECL_STATIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR

#define DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(type_size, type) \
	template <size_t SIZE> dynamic_log_size_helper<SIZE + type_size> \
	operator << (const dynamic_log_size_helper<SIZE>& helper, const type&) \
	{ return dynamic_log_size_helper<SIZE + type_size>(helper.size); }

DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::BOOL, bool);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::CHAR, char);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::UINT8, uint8_t);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::INT16, int16_t);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::UINT16, uint16_t);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::INT32, int32_t);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::UINT32, uint32_t);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::INT64, int64_t);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::UINT64, uint64_t);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::FLOAT, float);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::DOUBLE, double);
DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR(estimated_size::LONGDOUBLE, long double);

#undef DECL_DYNAMIC_LOG_SIZE_HELPER_BASIC_TYPE_OPERATOR


// declare overlap for matching char array
// match:
//   char a[100];
//   const char const_a[100];
//   static_log_size_helper<0>() << a
//   static_log_size_helper<0>() << const_a
//   static_log_size_helper<0>() << "const string!"
//   dynamic_log_size_helper<0>() << "const string!"
template <size_t SIZE, size_t ARRAY_SIZE>
static_log_size_helper<SIZE + ARRAY_SIZE>
operator << (const static_log_size_helper<SIZE>&, const char (*)[ARRAY_SIZE])
{ return static_log_size_helper<SIZE + ARRAY_SIZE>(); }

template <size_t SIZE, size_t ARRAY_SIZE>
dynamic_log_size_helper<SIZE + ARRAY_SIZE>
operator << (const dynamic_log_size_helper<SIZE>& helper, const char (*)[ARRAY_SIZE])
{ return dynamic_log_size_helper<SIZE + ARRAY_SIZE>(helper.size); }

template <size_t SIZE, size_t ARRAY_SIZE>
static_log_size_helper<SIZE + ARRAY_SIZE>
operator << (const static_log_size_helper<SIZE>&, const char (&)[ARRAY_SIZE])
{ return static_log_size_helper<SIZE + ARRAY_SIZE>(); }

template <size_t SIZE, size_t ARRAY_SIZE>
dynamic_log_size_helper<SIZE + ARRAY_SIZE>
operator << (const dynamic_log_size_helper<SIZE>& helper, const char (&)[ARRAY_SIZE])
{ return dynamic_log_size_helper<SIZE + ARRAY_SIZE>(helper.size); }

// declare overlap for matching pointers
// match:
//   int* a = NULL;
//   std::string* b = NULL;
//   static_log_size_helper<0>() << a
//   dynamic_log_size_helper<0>() << b
template <size_t SIZE, typename TYPE>
static_log_size_helper<SIZE + estimated_size::POINTER>
operator << (const static_log_size_helper<SIZE>&, const TYPE* s)
{ return static_log_size_helper<SIZE + estimated_size::POINTER>(); }

template <size_t SIZE, typename TYPE>
dynamic_log_size_helper<SIZE + estimated_size::POINTER>
operator << (const dynamic_log_size_helper<SIZE>& helper, const TYPE* s)
{ return dynamic_log_size_helper<SIZE + estimated_size::POINTER>(helper.size); }

// declare overlap for matching char pointer or const char pointer
// match:
//   char *a;
//   static_log_size_helper<0>() << a
//   dynamic_log_size_helper<0>() << a
template <size_t SIZE>
dynamic_log_size_helper<SIZE>
operator << (const static_log_size_helper<SIZE>&, const char* s)
{ return dynamic_log_size_helper<SIZE>(strlen(s)); }

template <size_t SIZE>
dynamic_log_size_helper<SIZE>
operator << (const dynamic_log_size_helper<SIZE>& helper, const char* s)
{ return dynamic_log_size_helper<SIZE>(helper.size + strlen(s)); }

// declare overlap for matching std::string
// match:
//   std::string a("hello world!");
//   static_log_size_helper<0>() << a
//   dynamic_log_size_helper<0>() << a
template <size_t SIZE>
dynamic_log_size_helper<SIZE>
operator << (const static_log_size_helper<SIZE>&, const std::string& s)
{ return dynamic_log_size_helper<SIZE>(s.length()); }

template <size_t SIZE>
dynamic_log_size_helper<SIZE>
operator << (const dynamic_log_size_helper<SIZE>& helper, const std::string& s)
{ return dynamic_log_size_helper<SIZE>(helper.size + s.length()); }


//////////////////////////////////////////////////////
// define static_checker
//////////////////////////////////////////////////////
struct static_checker {};

template <bool STATIC>
struct static_checker_helper;

template <>
struct static_checker_helper<true>
{
	int8_t dummy;
};

template <>
struct static_checker_helper<false>
{
	int16_t dummy;
};

STATIC_ASSERT(
		sizeof(static_checker_helper<true>) != sizeof(static_checker_helper<false>),
		should_have_different_size);

template <size_t SIZE>
const static_checker_helper<true>&
operator << (const static_log_size_helper<SIZE>&, const static_checker&);

template <size_t SIZE>
const static_checker_helper<false>&
operator << (const dynamic_log_size_helper<SIZE>&, const static_checker&);

//////////////////////////////////////////////////////
// define log_size_traits
//////////////////////////////////////////////////////
struct log_size_traits {};

template <size_t SIZE>
char (&operator << (const static_log_size_helper<SIZE>&, const log_size_traits&))[SIZE];

template <size_t SIZE>
size_t operator << (const dynamic_log_size_helper<SIZE>& helper, const log_size_traits&)
{ return SIZE + helper.size; }

///////////////////////////////////////////////////////////////

typedef static_log_size_helper<0> log_size_helper;

} // namespace logger
} // namespace sax

#endif /* _SAX_LOG_SIZE_HELPER_H_ */
