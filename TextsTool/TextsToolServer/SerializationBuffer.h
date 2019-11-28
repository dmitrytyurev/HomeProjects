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
	void PushUint8(uint8_t v)
	{
		PushInner<uint8_t>(v);
	}

	void PushUint32(uint32_t v)
	{
		PushInner<uint32_t>(v);
	}

	void PushUint64(uint64_t v)
	{
		PushInner<uint64_t>(v);
	}

	void PushString8(const std::string& v)
	{
		PushStringWithoutZero<uint8_t>(v);
	}

	void PushString16(const std::string& v)
	{
		PushStringWithoutZero<uint16_t>(v);
	}

	void PushVector8(std::vector<uint8_t>& v)
	{
		PushVector<uint8_t>(v);
	}

	void Push(const DeserializationBuffer& buf, bool useAllBuffer); // если useAllBuffer == true, то используем весь буффер, иначе только то, что ещё не прочитано из буфера
	void PushStringWithoutZero(const std::string& v);
	void PushBytes(const void* bytes, int size);
	int GetSize() { return buffer.size(); }
	uint8_t* GetData() { return buffer.data(); }

	std::vector<uint8_t> buffer;

private:
	template <typename T>
	void PushInner(T v);
	template <typename T>
	void PushStringWithoutZero(const std::string& v);
	template <typename T>
	void PushVector(std::vector<uint8_t>& v);
};

using SerializationBufferPtr = std::shared_ptr<SerializationBuffer>;


//===============================================================================

template <typename T>
void SerializationBuffer::PushInner(T v)
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
	PushInner<T>(nameLength);

	if (nameLength) {
		const uint8_t* beg = reinterpret_cast<const uint8_t*>(v.c_str());
		const uint8_t* end = beg + v.length();

		buffer.insert(buffer.end(), beg, end);
	}
}

//===============================================================================

template <typename T>
void SerializationBuffer::PushVector(std::vector<uint8_t>& v)
{
	T vectorSize = static_cast<T>(v.size());
	PushInner<T>(vectorSize);

	if (vectorSize) {
		const uint8_t* beg = reinterpret_cast<const uint8_t*>(v.data());
		const uint8_t* end = beg + v.size();

		buffer.insert(buffer.end(), beg, end);
	}
}
