#ifndef __VALIDATE_2008__
#define __VALIDATE_2008__

/// @file validate.h
/// check validity duing compile time (adapted from Loki).

#ifndef STATIC_ASSERT

namespace sax {

template <bool> struct __compile_time_error;
template <>     struct __compile_time_error<true> {};

} //namespace

#define STATIC_ASSERT(_Expr, _Msg) \
do { \
	sax::__compile_time_error<((_Expr) != 0)> ERROR_##_Msg; \
	(void) ERROR_##_Msg; \
} while(0)

#endif //STATIC_ASSERT


#endif //__VALIDATE_2008__
