#include "SerializationBuffer.h"

//===============================================================================

void SerializationBuffer::PushStringWithoutZero(const std::string& v)
{
	const uint8_t* beg = reinterpret_cast<const uint8_t*>(v.c_str());
	const uint8_t* end = beg + v.length();

	buffer.insert(buffer.end(), beg, end);
}

//===============================================================================

void SerializationBuffer::Push(const DeserializationBuffer& buf, bool useAllBuffer)
{
	const uint8_t* beg = buf._buffer.data() + (useAllBuffer ? 0 : buf.offset);
	const uint8_t* end = buf._buffer.data() + buf._buffer.size();

	buffer.insert(buffer.end(), beg, end);
}

//===============================================================================

void SerializationBuffer::PushBytes(const void* bytes, int size)
{
	const uint8_t* beg = reinterpret_cast<const uint8_t*>(bytes);
	const uint8_t* end = beg + size;

	buffer.insert(buffer.end(), beg, end);
}

