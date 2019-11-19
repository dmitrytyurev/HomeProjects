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
	template <typename T>
	void Push2(T v);

	template <typename T>
	void PushStringWithoutZero(const std::string& v);

	void Push(const DeserializationBuffer& buf, bool useAllBuffer); // если useAllBuffer == true, то используем весь буффер, иначе только то, что ещё не прочитано из буфера
	void PushStringWithoutZero(const std::string& v);
	void PushBytes(const void* bytes, int size);
	int GetSize() {	return buffer.size(); }
	uint8_t* GetData() { return buffer.data(); }

	std::vector<uint8_t> buffer;
};

using SerializationBufferPtr = std::shared_ptr<SerializationBuffer>;


//===============================================================================

template <typename T>
void SerializationBuffer::Push2(T v)
{
	const uint8_t* beg = reinterpret_cast<const uint8_t*>(&v);
	const uint8_t* end = beg + sizeof(T);

	buffer.insert(buffer.end(), beg, end);
}

//===============================================================================

template <typename T>
void SerializationBuffer::PushStringWithoutZero(const std::string& v)
{
	T nameLength = static_cast<T>(v.length());
	Push2<T>(nameLength);

	if (nameLength) {
		const uint8_t* beg = reinterpret_cast<const uint8_t*>(v.c_str());
		const uint8_t* end = beg + v.length();

		buffer.insert(buffer.end(), beg, end);
	}
}

