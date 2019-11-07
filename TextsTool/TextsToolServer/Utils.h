#pragma once

#include <mutex>

void ExitMsg(const std::string& message);
void Log(const std::string& message);
uint32_t GetTime();
uint64_t AddHash(uint64_t curHash, std::vector<uint8_t>& key, bool isFirstPart);  // FNV-1a algorithm https://softwareengineering.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed

class MutexLock
{
public:
	MutexLock(std::mutex& mutex) { pMutex = &mutex; mutex.lock(); }
	~MutexLock() { pMutex->unlock(); }
private:
	std::mutex* pMutex = nullptr;
};

