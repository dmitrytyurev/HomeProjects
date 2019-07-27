#pragma once

//===============================================================================
// 
//===============================================================================

class DeserializationBuffer
{
public:
	template <typename T>
	T GetUint();
	template <typename T>
	void GetString(std::string& result);
	bool IsEmpty() { return buffer.size() - 1 == offset; }

	uint32_t offset = 0;
	std::vector<uint8_t> buffer;
};

//===============================================================================
//
//===============================================================================

template <typename T>
void DeserializationBuffer::GetString(std::string& result)
{
#if CHECK_BUFFER
	if (offset + sizeof(T) > buffer.size()) {
		ExitMsg("GetString buffer overrun");
	}
#endif
	T textLength = *(reinterpret_cast<T*>(buffer.data() + offset));
	offset += sizeof(T);
	uint8_t keep = buffer[offset + textLength];
	buffer[offset + textLength] = 0;

#if CHECK_BUFFER
	if (offset + 1 > buffer.size()) {
		ExitMsg("GetString buffer overrun");
	}
#endif
	result = reinterpret_cast<const char*>(buffer.data() + offset);
	buffer[offset + textLength] = keep;
	offset += textLength;
}


//===============================================================================
//
//===============================================================================

template <typename T>
T DeserializationBuffer::GetUint()
{
#if CHECK_BUFFER
	if (offset + sizeof(T) > buffer.size()) {
		ExitMsg("GetUint buffer overrun");
	}
#endif
	T val = *(reinterpret_cast<T*>(buffer.data() + offset));
	offset += sizeof(T);
	return val;
}
