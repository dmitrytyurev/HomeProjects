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
	DeserializationBuffer(const uint8_t* buf, int bufSize);
	void AddBytes(const uint8_t* buf, int bufSize);

	template <typename T>
	T GetUint();
	template <typename T>
	void GetString(std::string& result);
	template <typename T>
	std::vector<uint8_t> GetVector();
	bool IsEmpty() { return _buffer.size() == offset; }
	uint8_t* GetNextBytes(uint32_t size) { offset += size; return  _buffer.data() + offset - size; }
	int GetRestBytesNum() { return _buffer.size() - offset; }

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
#if CHECK_BUFFER
	if (offset + textLength >= _buffer.size()) {
		ExitMsg("GetString _buffer overrun");
	}
#endif
	uint8_t keep = _buffer[offset + textLength];
	_buffer[offset + textLength] = 0;
	result = reinterpret_cast<const char*>(_buffer.data() + offset);
	_buffer[offset + textLength] = keep;
	offset += textLength;
}

//===============================================================================
//
//===============================================================================

template <typename T>
std::vector<uint8_t> DeserializationBuffer::GetVector()
{
#if CHECK_BUFFER
	if (offset + sizeof(T) > _buffer.size()) {
		ExitMsg("GetVector _buffer overrun");
	}
#endif
	T vectLength = *(reinterpret_cast<T*>(_buffer.data() + offset));
	offset += sizeof(T);
#if CHECK_BUFFER
	if (offset + vectLength > _buffer.size()) {
		ExitMsg("GetVector _buffer overrun");
	}
#endif

	offset += vectLength;
	return std::vector<uint8_t>(_buffer.data() + offset - vectLength, _buffer.data() + offset);
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
