#include "pch.h"

#include "DeserializationBuffer.h"

//===============================================================================
//
//===============================================================================

DeserializationBuffer::DeserializationBuffer(std::vector<uint8_t>& buffer): _buffer(buffer)
{
}

//===============================================================================
//
//===============================================================================

DeserializationBuffer::DeserializationBuffer(const uint8_t* buf, int bufSize)
{
	_buffer.resize(bufSize);
	memcpy(_buffer.data(), buf, bufSize);
}