/*
 * thrift_binary_protocol.h
 *
 *  Created on: 2012-9-4
 *      Author: x
 */

#ifndef _THRIFT_BINARY_PROTOCOL_H_
#define _THRIFT_BINARY_PROTOCOL_H_

/**
 * base on thrift-0.8 "protocol/TProtocol.h",
 * compatible with the thrift binary protocol
 */

#include <limits>

#include "buffer.h"
#include "sax/compiler.h"
#include "sax/os_types.h"
#include "sax/cpputil.h"    // for bitwise_cast()

namespace sax {

/**
 * Enumerated definition of the types that the Thrift protocol supports.
 * Take special note of the T_END type which is used specifically to mark
 * the end of a sequence of fields.
 */
enum TType
{
	T_STOP       = 0,
	T_VOID       = 1,
	T_BOOL       = 2,
	T_BYTE       = 3,
	T_I08        = 3,
	T_I16        = 6,
	T_I32        = 8,
	T_U64        = 9,
	T_I64        = 10,
	T_DOUBLE     = 4,
	T_STRING     = 11,
	T_UTF7       = 11,
	T_STRUCT     = 12,
	T_MAP        = 13,
	T_SET        = 14,
	T_LIST       = 15,
	T_UTF8       = 16,
	T_UTF16      = 17
};

/**
 * Enumerated definition of the message types that the Thrift protocol
 * supports.
 */
enum TMessageType
{
	T_CALL       = 1,
	T_REPLY      = 2,
	T_EXCEPTION  = 3,
	T_ONEWAY     = 4
};

// TODO: limit string size and container size

class thrift_binary_protocol
{
	static const int32_t VERSION_MASK = 0xffff0000;
	static const int32_t VERSION_1 = 0x80010000;

public:
	thrift_binary_protocol(buffer* buf) : _buf(buf) {}
	~thrift_binary_protocol() {}

	uint32_t writeMessageBegin(const std::string& name,
							   const TMessageType messageType,
							   const int32_t seqid)
	{
		// always strict write
		int32_t version = (VERSION_1) | ((int32_t)messageType);
		uint32_t wsize = 0;
		wsize += writeI32(version);
		wsize += writeString(name);
		wsize += writeI32(seqid);
		return wsize;
	}

	uint32_t writeMessageEnd()
	{
		return 0;
	}


	uint32_t writeStructBegin(const char*)
	{
		return 0;
	}

	uint32_t writeStructEnd()
	{
		return 0;
	}

	uint32_t writeFieldBegin(const char*,
						     const TType fieldType,
						     const int16_t fieldId)
	{
		uint32_t wsize = 0;
		wsize += writeByte((int8_t)fieldType);
		wsize += writeI16(fieldId);
		return wsize;
	}

	uint32_t writeFieldEnd()
	{
		return 0;
	}

	uint32_t writeFieldStop()
	{
		return writeByte((int8_t)T_STOP);
	}

	uint32_t writeMapBegin(const TType keyType,
						   const TType valType,
						   const uint32_t size)
	{
		uint32_t wsize = 0;
		wsize += writeByte((int8_t)keyType);
		wsize += writeByte((int8_t)valType);
		wsize += writeI32((int32_t)size);
		return wsize;
	}

	uint32_t writeMapEnd()
	{
		return 0;
	}

	uint32_t writeListBegin(const TType elemType, const uint32_t size)
	{
		uint32_t wsize = 0;
		wsize += writeByte((int8_t)elemType);
		wsize += writeI32((int32_t)size);
		return wsize;
	}

	uint32_t writeListEnd()
	{
		return 0;
	}

	uint32_t writeSetBegin(const TType elemType, const uint32_t size)
	{
		uint32_t wsize = 0;
		wsize += writeByte((int8_t)elemType);
		wsize += writeI32((int32_t)size);
		return wsize;
	}

	uint32_t writeSetEnd()
	{
		return 0;
	}

	uint32_t writeBool(const bool value)
	{
		_buf->put((uint8_t)(value & 0x00000001));
		return 1;
	}

	uint32_t writeByte(const int8_t byte)
	{
		_buf->put((uint8_t)byte);
		return 1;
	}

	uint32_t writeI16(const int16_t i16)
	{
		_buf->put((uint16_t)i16, true);
		return 2;
	}

	uint32_t writeI32(const int32_t i32)
	{
		_buf->put((uint32_t)i32, true);
		return 4;
	}

	uint32_t writeI64(const int64_t i64)
	{
		_buf->put((uint64_t)i64, true);
		return 8;
	}

	uint32_t writeDouble(const double dub)
	{
		STATIC_ASSERT(sizeof(double) == sizeof(uint64_t), double_is_not_8_bytes);
		STATIC_ASSERT(std::numeric_limits<double>::is_iec559, not_iec559);

		uint64_t bits = bitwise_cast<uint64_t>(dub);
		_buf->put(bits, true);
		return 8;
	}

	uint32_t writeString(const std::string& str)
	{
		uint32_t size = (uint32_t)str.size();
		uint32_t result = writeI32((int32_t)size);
		if (size > 0) {
			_buf->put((uint8_t*)str.c_str(), size);
		}
		return result + size;
	}

	uint32_t writeBinary(const std::string& str)
	{
		return writeString(str);
	}

	uint32_t readMessageBegin(std::string& name,
							  TMessageType& messageType,
							  int32_t& seqid)
	{
		uint32_t result = 0;
		int32_t sz;
		result += readI32(sz);

		// Check for correct version number
		int32_t version = sz & VERSION_MASK;
		if (UNLIKELY(version != VERSION_1)) {
			// TODO: no throw
			throw std::exception();	// Bad version identifier
		}

		messageType = (TMessageType)(sz & 0x000000ff);
		result += readString(name);
		result += readI32(seqid);

		return result;
	}

	uint32_t readMessageEnd()
	{
		return 0;
	}

	uint32_t readStructBegin(std::string& name)
	{
		name.clear();
		return 0;
	}

	uint32_t readStructEnd()
	{
		return 0;
	}

	uint32_t readFieldBegin(std::string&,
						    TType& fieldType,
						    int16_t& fieldId)
	{
		uint32_t result = 0;
		int8_t type;
		result += readByte(type);
		fieldType = (TType)type;
		if (fieldType == T_STOP) {
			fieldId = 0;
			return result;
		}
		result += readI16(fieldId);
		return result;
	}

	uint32_t readFieldEnd()
	{
		return 0;
	}

	uint32_t readMapBegin(TType& keyType, TType& valType, uint32_t& size)
	{
		int8_t k, v;
		uint32_t result = 0;
		int32_t sizei;
		result += readByte(k);
		keyType = (TType)k;
		result += readByte(v);
		valType = (TType)v;
		result += readI32(sizei);
		size = (uint32_t)sizei;
		return result;
	}

	uint32_t readMapEnd()
	{
		return 0;
	}

	uint32_t readListBegin(TType& elemType, uint32_t& size)
	{
		int8_t e;
		uint32_t result = 0;
		int32_t sizei;
		result += readByte(e);
		elemType = (TType)e;
		result += readI32(sizei);
		size = (uint32_t)sizei;
		return result;
	}

	uint32_t readListEnd()
	{
		return 0;
	}

	uint32_t readSetBegin(TType& elemType, uint32_t& size)
	{
		int8_t e;
		uint32_t result = 0;
		int32_t sizei;
		result += readByte(e);
		elemType = (TType)e;
		result += readI32(sizei);
		size = (uint32_t)sizei;
		return result;
	}

	uint32_t readSetEnd()
	{
		return 0;
	}

	uint32_t readBool(bool& value)
	{
		uint8_t b;
		_buf->get(b);
		value = b != 0;
		return 1;
	}

	uint32_t readByte(int8_t& byte)
	{
	    // TODO: this implementation will break the strict aliasing rule
		_buf->get(static_cast<uint8_t&>(byte));
		return 1;
	}

	uint32_t readI16(int16_t& i16)
	{
	    // TODO: this implementation will break the strict aliasing rule
		_buf->get(static_cast<uint16_t&>(i16), true);
		return 2;
	}

	uint32_t readI32(int32_t& i32)
	{
	    // TODO: this implementation will break the strict aliasing rule
		_buf->get(static_cast<uint32_t&>(i32), true);
		return 4;
	}

	uint32_t readI64(int64_t& i64)
	{
	    // TODO: this implementation will break the strict aliasing rule
		_buf->get(static_cast<uint64_t&>(i64), true);
		return 8;
	}

	uint32_t readDouble(double& dub)
	{
	    // TODO: this implementation will break the strict aliasing rule
		STATIC_ASSERT(sizeof(double) == sizeof(uint64_t), double_is_not_8_bytes);
		STATIC_ASSERT(std::numeric_limits<double>::is_iec559, not_iec559);

		uint64_t bits;
		_buf->get(bits, true);
		dub = bitwise_cast<double>(bits);
		return 8;
	}

	uint32_t readString(std::string& str)
	{
		uint32_t result;
		int32_t size;
		result = readI32(size);
		return result + readStringBody(str, size);
	}

	uint32_t readBinary(std::string& str)
	{
		return readString(str);
	}

	/*
	* std::vector is specialized for bool, and its elements are individual bits
	* rather than bools.   We need to define a different version of readBool()
	* to work with std::vector<bool>.
	*/
	uint32_t readBool(std::vector<bool>::reference value)
	{
		bool b = false;
		uint32_t ret = readBool(b);
		value = b;
		return ret;
	}

	/**
	* Method to arbitrarily skip over data.
	*/
	uint32_t skip(TType type)
	{
		switch (type) {
		case T_BOOL:
		{
			_buf->skip(1);
			return 1;
		}
		case T_BYTE:
		{
			_buf->skip(1);
			return 1;
		}
		case T_I16:
		{
			_buf->skip(2);
			return 2;
		}
		case T_I32:
		{
			_buf->skip(4);
			return 4;
		}
		case T_I64:
		{
			_buf->skip(8);
			return 8;
		}
		case T_DOUBLE:
		{
			_buf->skip(8);
			return 8;
		}
		case T_STRING:
		{
			int32_t size;
			uint32_t result = readI32(size);
			_buf->skip(size);
			return result + (uint32_t)size;
		}
		case T_STRUCT:
		{
			uint32_t result = 0;
			std::string name;
			int16_t fid;
			TType ftype;
			//result += readStructBegin(name);	// do nothing in readStructBegin() for binary protocol
			while (true) {
				result += readFieldBegin(name, ftype, fid);
				if (ftype == T_STOP) {
					break;
				}
				result += skip(ftype);
				//result += readFieldEnd();	// do nothing in readFieldEnd() for binary protocol
			}
			//result += readStructEnd();	// do nothing in readFieldEnd() for binary protocol
			return result;
		}
		case T_MAP:
		{
			uint32_t result = 0;
			TType keyType;
			TType valType;
			uint32_t i, size;
			result += readMapBegin(keyType, valType, size);
			for (i = 0; i < size; i++) {
				result += skip(keyType);
				result += skip(valType);
			}
			//result += readMapEnd();	// do nothing in readMapEnd() for binary protocol
			return result;
		}
		case T_SET:
		{
			uint32_t result = 0;
			TType elemType;
			uint32_t i, size;
			result += readSetBegin(elemType, size);
			for (i = 0; i < size; i++) {
				result += skip(elemType);
			}
			//result += readSetEnd();	// do nothing in readSetEnd() for binary protocol
			return result;
		}
		case T_LIST:
		{
			uint32_t result = 0;
			TType elemType;
			uint32_t i, size;
			result += readListBegin(elemType, size);
			for (i = 0; i < size; i++) {
				result += skip(elemType);
			}
			//result += readListEnd();	// do nothing in readSetEnd() for binary protocol
			return result;
		}
		case T_STOP:
		case T_VOID:
		case T_U64:
		case T_UTF8:
		case T_UTF16:
			break;
		}
		return 0;
	}

private:
	uint32_t readStringBody(std::string& str, uint32_t size)
	{
		uint32_t result = 0;

		// Catch empty string case
		if (size == 0) {
			str = "";
			return result;
		}

		// trick
		str.resize(size);
		_buf->get((uint8_t*)str.c_str(), size);
		return (uint32_t)size;
	}

private:
	buffer* _buf;
};

} // namespace sax

#endif /* _THRIFT_BINARY_PROTOCOL_H_ */
