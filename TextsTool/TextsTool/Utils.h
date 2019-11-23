#pragma once

#include <mutex>
#include <vector>

void ExitMsg(const std::string& message);
void Log(const std::string& str);

namespace Utils
{
	void LogBuf(std::vector<uint8_t>& buffer);
	uint GetCurrentTimestamp();
}
