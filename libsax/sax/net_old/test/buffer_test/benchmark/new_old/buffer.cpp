/*
 * buffer.cpp
 *
 *  Created on: 2012-3-29
 *      Author: X
 */

#include "./buffer.h"

namespace sax {
namespace newold {

uint32_t const sax::newold::buffer::__TEST_ENDIAN_UINT32_T__ = 0x12345678;
bool const sax::newold::buffer::IS_BIGENDIAN = reinterpret_cast<const volatile char&>(
		sax::newold::buffer::__TEST_ENDIAN_UINT32_T__) == static_cast<char>(0x012);

}
}
