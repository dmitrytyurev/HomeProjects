#pragma once

#include <string>
#include <vector>

#include "DeserializationBuffer.h"

//===============================================================================
// 
//===============================================================================

class SerializationBuffer
{
public:
	void Push(uint8_t v);
	void Push(uint16_t v);
	void Push(uint32_t v);
	void Push(const DeserializationBuffer& buf);
	//	void Push(const std::string& v);
	void PushStringWithoutZero(const std::string& v);

	template <typename T>
	void PushStringWithoutZero(const std::string& v);


	void PushBytes(const void* bytes, int size);

	std::vector<uint8_t> buffer;
};

using SerializationBufferPtr = std::shared_ptr<SerializationBuffer>;


//===============================================================================
//
//===============================================================================

template <typename T>
void SerializationBuffer::PushStringWithoutZero(const std::string& v)
{
	T nameLength = static_cast<T>(v.length());
	Push(nameLength);

	const uint8_t* beg = reinterpret_cast<const uint8_t*>(v.c_str());
	const uint8_t* end = beg + v.length();

	buffer.insert(buffer.end(), beg, end);
}
