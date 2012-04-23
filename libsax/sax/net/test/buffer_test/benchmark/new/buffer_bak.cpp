/*
 * buffer.cpp
 *
 *  Created on: 2012-3-29
 *      Author: X
 */

#include "buffer.h"

namespace sax {

uint32_t const buffer::__TEST_ENDIAN_UINT32_T__ = 0x12345678;
bool const buffer::IS_BIGENDIAN = reinterpret_cast<const volatile char&>(
		buffer::__TEST_ENDIAN_UINT32_T__) == static_cast<char>(0x012);

}
