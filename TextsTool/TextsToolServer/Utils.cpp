#include "pch.h"
#include <iostream>
#include <experimental/filesystem>

//===============================================================================
//
//===============================================================================

void ExitMsg(const std::string& message)
{
	// !!! Сделать запись в лог
	std::cout << "Fatal: " << message << std::endl;
	exit(1);
}

//===============================================================================
//
//===============================================================================

void LogMsg(const std::string& message)
{
	// !!! Сделать запись в лог
	std::cout << message << std::endl;
}

//===============================================================================
//
//===============================================================================

uint32_t GetTime()
{
	return static_cast<uint32_t>(std::time(0));
}


//===============================================================================
// FNV-1a algorithm https://softwareengineering.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed
//===============================================================================

uint64_t AddHash(uint64_t curHash, std::vector<uint8_t>& key, bool isFirstPart)
{
	if (isFirstPart) {
		curHash = 14695981039346656037;
	}

	for (uint8_t el : key) {
		curHash = curHash ^ el;
		curHash *= 1099511628211;
	}
	return curHash;
}


