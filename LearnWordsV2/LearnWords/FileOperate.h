#pragma once
#include <vector>
#include "WordsManager.h"

struct FileOperate
{
	static void load_from_file(const char* fullFileName, std::vector<WordsManager::WordInfo>& wordsInfo);
	static void save_to_file(const char* fullFileName, const std::vector<WordsManager::WordInfo>& wordsInfo);

private:
	static std::string load_string_from_array(const std::vector<char>& buffer, int* indexToRead);
	static int load_int_from_array(const std::vector<char>& buffer, int* indexToRead);
};

