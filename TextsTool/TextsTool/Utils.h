#pragma once

#include <mutex>
#include <vector>

void ExitMsg(const std::string& message);
void Log(const std::string& str);

namespace Utils
{
	void LogBuf(std::vector<uint8_t>& buffer);
	uint32_t GetCurrentTimestamp();
	uint64_t AddHash(uint64_t curHash, std::vector<uint8_t>& key, bool isFirstPart);  // FNV-1a algorithm https://softwareengineering.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed
	std::string ConvertTimestampToDate(uint32_t timestamp);
}
