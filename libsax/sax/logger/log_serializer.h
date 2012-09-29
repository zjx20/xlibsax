/*
 * log_serizlizer.h
 *
 *  Created on: 2012-8-22
 *      Author: x
 */

#ifndef _SAX_LOG_SERIZLIZER_H_
#define _SAX_LOG_SERIZLIZER_H_

#include <cstdio>
#include <cstring>
#include <string>
#include <limits>
#include <cassert>
#include "sax/os_types.h"
#include "sax/compiler.h"
#include "log_size_helper.h"

namespace sax {
namespace logger {

#define TRUNCATED_MESSAGE " ...[this log has been truncated]\n"

// NOTICE: must write something before calling end_of_line() and
//         the size of given buffer must large enough for
//         writing the truncated warning
class log_serializer
{
public:
	log_serializer(char* buf, int32_t buf_size) :
		_buf(buf), _space(buf_size)
	{}

	inline void write(const char* data, int32_t size)
	{
		if (UNLIKELY(size > _space)) {
			size = _space;
		}

		memcpy(_buf, data, size);
		_buf += size;
		_space -= size;
	}

	inline char* end_of_line()
	{
		if (UNLIKELY(_space == 0)) {
			memcpy((_buf - sizeof(TRUNCATED_MESSAGE) - 1),
					TRUNCATED_MESSAGE, sizeof(TRUNCATED_MESSAGE) - 1);
		}
		else if (LIKELY(*(_buf - 1) != '\n')) {
			*_buf = '\n';
			++_buf;
		}
		return _buf;
	}

private:
	char* _buf;
	int32_t _space;
};

#undef TRUNCATED_MESSAGE

template <typename UINTTYPE>
static inline int32_t uint2str(char* buf, UINTTYPE n)
{
	STATIC_ASSERT(!std::numeric_limits<UINTTYPE>::is_signed,
			only_accept_unsigned_type);

	char* head = buf;
	do {
		*buf = (char) (n % 10) + '0';
		++buf;
		n /= 10;
	} while (n != 0);

	int32_t writed = buf - head;

	char tmp;
	while (head < --buf) {
		tmp = *head;
		*head = *buf;
		*buf = tmp;
		++head;
	}

	return writed;
}

template <typename UINTTYPE>
static inline int32_t uint2hexstr(char* buf, UINTTYPE n)
{
	STATIC_ASSERT(!std::numeric_limits<UINTTYPE>::is_signed,
			only_accept_unsigned_type);

	const char *charset = "0123456789ABCDEF";

	char* head = buf;
	do {
		*buf = charset[n % 16];
		++buf;
		n /= 16;
	} while (n != 0);

	int32_t writed = buf - head;

	char tmp;
	while (head < --buf) {
		tmp = *head;
		*head = *buf;
		*buf = tmp;
		++head;
	}

	return writed;
}

inline
log_serializer& operator << (log_serializer& serializer, const bool& b)
{
	serializer.write(b ? "1" : "0", 1);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const char& c)
{
	serializer.write(&c, 1);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const uint8_t& u8)
{
	char buf[estimated_size::UINT8];
	int32_t len = uint2str<uint8_t>(buf, u8);
	serializer.write(buf, len);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const int16_t& i16)
{
	char buf[estimated_size::INT16];
	if (i16 >= 0) {
		int32_t len = uint2str<uint16_t>(buf, (uint16_t) i16);
		serializer.write(buf, len);
	}
	else {
		buf[0] = '-';
		uint32_t u16 = (~((uint16_t) i16)) + 1;
		int32_t len = uint2str<uint16_t>(buf + 1, u16);
		serializer.write(buf, len + 1);
	}
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const uint16_t& u16)
{
	char buf[estimated_size::UINT16];
	int32_t len = uint2str<uint16_t>(buf, u16);
	serializer.write(buf, len);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const int32_t& i32)
{
	char buf[estimated_size::INT32];
	if (i32 >= 0) {
		int32_t len = uint2str<uint32_t>(buf, (uint32_t) i32);
		serializer.write(buf, len);
	}
	else {
		buf[0] = '-';
		uint32_t u32 = (~((uint32_t) i32)) + 1;
		int32_t len = uint2str<uint32_t>(buf + 1, u32);
		serializer.write(buf, len + 1);
	}
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const uint32_t& u32)
{
	char buf[estimated_size::UINT32];
	int32_t len = uint2str<uint32_t>(buf, u32);
	serializer.write(buf, len);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const int64_t& i64)
{
	char buf[estimated_size::INT64];
	if (i64 >= 0) {
		int32_t len = uint2str<uint64_t>(buf, (uint64_t) i64);
		serializer.write(buf, len);
	}
	else {
		buf[0] = '-';
		uint64_t u64 = (~((uint64_t) i64)) + 1;
		int32_t len = uint2str<uint64_t>(buf + 1, u64);
		serializer.write(buf, len + 1);
	}
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const uint64_t& u64)
{
	char buf[estimated_size::UINT64];
	int32_t len = uint2str<uint64_t>(buf, u64);
	serializer.write(buf, len);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const void* p)
{
	char buf[estimated_size::POINTER];
	int32_t len = uint2hexstr<uintptr_t>(buf, (uintptr_t) p);
	// write prefix and padding zero, support 128 bit pointer at most
	serializer.write("0x00000000000000000000000000000000",
			2 + sizeof(void*) * 2 - len);
	serializer.write(buf, len);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const float& f)
{
	char buf[estimated_size::FLOAT + 1];
	int32_t len = sprintf(buf, "%g", f);
	serializer.write(buf, len);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const double& d)
{
	char buf[estimated_size::DOUBLE + 1];
	int32_t len = sprintf(buf, "%g", d);
	serializer.write(buf, len);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const long double& ld)
{
	char buf[estimated_size::LONGDOUBLE + 1];
	int32_t len = sprintf(buf, "%Lg", ld);
	serializer.write(buf, len);
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const char* s)
{
	serializer.write(s, (int32_t) strlen(s));
	return serializer;
}

inline
log_serializer& operator << (log_serializer& serializer, const std::string& str)
{
	serializer.write(str.c_str(), (int32_t) str.length());
	return serializer;
}

} // namespace logger
} // namespace sax

#endif /* _SAX_LOG_SERIZLIZER_H_ */
