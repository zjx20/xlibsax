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
#include <cassert>
#include "sax/os_types.h"

namespace sax {
namespace logger {

class log_serializer
{
public:
	log_serializer(char* buf) :
		_buf(buf), _curr_pos(0)
	{}

	inline char* current() { return _buf + _curr_pos; }
	inline void advance(int n) { _curr_pos += n; }
	inline size_t length() { return _curr_pos; }
	inline char* data() { return _buf; }

private:
	char* _buf;
	size_t _curr_pos;
};

log_serializer& operator << (log_serializer& serializer, const char& c)
{
	*serializer.current() = c;
	serializer.advance(1);
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const uint8_t& i)
{
	serializer.advance(sprintf(serializer.current(), "%"PRIu8, i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const int16_t& i)
{
	serializer.advance(sprintf(serializer.current(), "%"PRId16, i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const uint16_t& i)
{
	serializer.advance(sprintf(serializer.current(), "%"PRIu16, i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const int32_t& i)
{
	serializer.advance(sprintf(serializer.current(), "%"PRId32, i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const uint32_t& i)
{
	serializer.advance(sprintf(serializer.current(), "%"PRIu32, i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const int64_t& i)
{
	serializer.advance(sprintf(serializer.current(), "%"PRId64, i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const uint64_t& i)
{
	serializer.advance(sprintf(serializer.current(), "%"PRIu64, i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const float& i)
{
	serializer.advance(sprintf(serializer.current(), "%g", i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const double& i)
{
	serializer.advance(sprintf(serializer.current(), "%g", i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const long double& i)
{
	serializer.advance(sprintf(serializer.current(), "%Lg", i));
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const char* s)
{
	size_t len = strlen(s);
	memcpy(serializer.current(), s, len);
	serializer.advance((int) len);
	return serializer;
}

log_serializer& operator << (log_serializer& serializer, const std::string& s)
{
	memcpy(serializer.current(), s.c_str(), s.length());
	serializer.advance((int) s.length());
	return serializer;
}

class log_serializer_eol {};

log_serializer& operator << (log_serializer& serializer, const log_serializer_eol& s)
{
	assert(serializer.length() > 0);
	if (*(serializer.current() - 1) != '\n') {
		*serializer.current() = '\n';
		serializer.advance(1);
	}
	*serializer.current() = '\0';
	return serializer;
}

} // namespace logger
} // namespace sax

#endif /* _SAX_LOG_SERIZLIZER_H_ */
