#include "stdafx.h"
#include <string>
#include <fstream>
#include <Windows.h>

#include "FileOperate.h"
#include "CommonUtility.h"
#include "WordsData.h"


//===============================================================================================
// 
//===============================================================================================

std::string FileOperate::load_string_from_array(const std::vector<char>& buffer, int* indexToRead)
{
	while (buffer[*indexToRead] != '"'  &&  buffer[*indexToRead] != 0)
		++(*indexToRead);
	if (buffer[*indexToRead] == 0)
		return std::string("");
	++(*indexToRead);

	std::string str;
	str.reserve(100);

	while (buffer[*indexToRead] != '"')
	{
		str += buffer[*indexToRead];
		++(*indexToRead);
	}
	++(*indexToRead);

	return str;
}

//===============================================================================================
// 
//===============================================================================================

int FileOperate::load_int_from_array(const std::vector<char>& buffer, int* indexToRead)
{
	int number = 0;

	while (true)
	{
		number += buffer[*indexToRead] - '0';
		++(*indexToRead);
		if (!is_digit(buffer[*indexToRead]))
			return number;
		number *= 10;
	}
	return 0;
}

//===============================================================================================
// 
//===============================================================================================

void FileOperate::load_from_file(const char* fullFileName, WordsData* pWordsData)
{
	std::ifstream file(fullFileName, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	if (size == -1)
		exit_msg("Error opening file %s\n", fullFileName);
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer((int)size + 1);
	buffer[(int)size] = 0;
	if (!file.read(buffer.data(), size))
		exit_msg("Error reading file %s\n", fullFileName);

	int parseIndex = 0;


	// Читаем основной блок слов
	while (true)
	{
		WordsData::WordInfo wi;

		wi.word = load_string_from_array(buffer, &parseIndex);
		if (wi.word.length() == 0)         // При поиске начала строки был встречен конец файла (например, файл заканчивается пустой строкой)
			return;
		wi.translation = load_string_from_array(buffer, &parseIndex);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
			++parseIndex;

		if (is_digit(buffer[parseIndex]))  // Читаем параметры
		{
			wi.checkOrderN = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());
			// ---------
			wi.successCheckDays = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());
			// ---------
			wi.lastDaySuccCheckTimestamp = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());

			// ---------
			wi.needSkip = !!load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
				++parseIndex;
		}

		// Занести WordInfo в вектор
		pWordsData->_words.push_back(wi);

		if (buffer[parseIndex] == 0)
			return;
	}
}

//===============================================================================================
// 
//===============================================================================================

void FileOperate::save_to_file(const char* fullFileName, WordsData* pWordsData)
{
	const int NUM_TRIES = 10;
	const int DURATION_OF_TRIES = 1000;

	FILE* f = nullptr;
	int i = 0;
	for (i = 0; i < NUM_TRIES; i++)
	{
		fopen_s(&f, fullFileName, "wt");
		if (f)
			break;
		Sleep(DURATION_OF_TRIES / NUM_TRIES);
	}
	if (i == NUM_TRIES)
		exit_msg("Can't create file %s\n", fullFileName);

	for (const auto& e : pWordsData->_words)
	{
		if (e.successCheckDays == 0)
			fprintf(f, "\"%s\" \"%s\"\n", e.word.c_str(), e.translation.c_str());
		else
		{
			fprintf(f, "\"%s\" \"%s\" %d %d %d %d",
				e.word.c_str(),
				e.translation.c_str(),
				e.checkOrderN,
				e.successCheckDays,
				e.lastDaySuccCheckTimestamp,
				int(e.needSkip));

			fprintf(f, "\n");
		}
	}
	fclose(f);
}

