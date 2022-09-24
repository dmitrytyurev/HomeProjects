#pragma once
#include <vector>
#include "WordsManager.h"

struct FileOperate
{
	static void LoadFromFile(const char* fullFileName, std::vector<WordsManager::WordInfo>& wordsInfo);
	static void SaveToFile(const char* fullFileName, const std::vector<WordsManager::WordInfo>& wordsInfo);

private:
	static std::string LoadStringFromArray(const std::vector<char>& buffer, int* indexToRead);
	static int LoadIntFromArray(const std::vector<char>& buffer, int* indexToRead);
};

