#pragma once
#include <vector>

struct WordsData;

struct FileOperate
{
	static void load_from_file(const char* fullFileName, WordsData* pWordsData);
	static void save_to_file(const char* fullFileName, WordsData* pWordsData);
	static void export_for_google_doc(WordsData* pWordsData);

private:
	static std::string load_string_from_array(const std::vector<char>& buffer, int* indexToRead);
	static int load_int_from_array(const std::vector<char>& buffer, int* indexToRead);
};

