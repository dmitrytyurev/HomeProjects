#include "pch.h"

#include "DeserializationBuffer.h"

//===============================================================================

DeserializationBuffer::DeserializationBuffer(const std::vector<uint8_t>& buffer): _buffer(buffer)
{
}

//===============================================================================

DeserializationBuffer::DeserializationBuffer(const uint8_t* buf, int bufSize)
{
	_buffer.resize(bufSize);
	memcpy(_buffer.data(), buf, bufSize);
}

//===============================================================================

void DeserializationBuffer::AddBytes(const uint8_t* buf, int bufSize)
{
	int size = _buffer.size();
	_buffer.resize(size + bufSize);
	memcpy(_buffer.data() + size, buf, bufSize);
}
