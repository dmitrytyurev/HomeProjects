#pragma once

#include <string>
#include <vector>

#define CHECK_BUFFER 1

void ExitMsg(const std::string& message);

//===============================================================================
// 
//===============================================================================

class DeserializationBuffer
{
public:
	using Ptr = std::unique_ptr<DeserializationBuffer>;

	DeserializationBuffer() {}
	DeserializationBuffer(std::vector<uint8_t>& buffer);
	template <typename T>
	T GetUint();
	template <typename T>
	void GetString(std::string& result);
	bool IsEmpty() { return _buffer.size() - 1 == offset; }

	uint32_t offset = 0;
	std::vector<uint8_t> _buffer;
};

//===============================================================================
//
//===============================================================================

template <typename T>
void DeserializationBuffer::GetString(std::string& result)
{
#if CHECK_BUFFER
	if (offset + sizeof(T) > _buffer.size()) {
		ExitMsg("GetString _buffer overrun");
	}
#endif
	T textLength = *(reinterpret_cast<T*>(_buffer.data() + offset));
	offset += sizeof(T);
	uint8_t keep = _buffer[offset + textLength];
	_buffer[offset + textLength] = 0;

#if CHECK_BUFFER
	if (offset + 1 > _buffer.size()) {
		ExitMsg("GetString _buffer overrun");
	}
#endif
	result = reinterpret_cast<const char*>(_buffer.data() + offset);
	_buffer[offset + textLength] = keep;
	offset += textLength;
}


//===============================================================================
//
//===============================================================================

template <typename T>
T DeserializationBuffer::GetUint()
{
#if CHECK_BUFFER
	if (offset + sizeof(T) > _buffer.size()) {
		ExitMsg("GetUint _buffer overrun");
	}
#endif
	T val = *(reinterpret_cast<T*>(_buffer.data() + offset));
	offset += sizeof(T);
	return val;
}
