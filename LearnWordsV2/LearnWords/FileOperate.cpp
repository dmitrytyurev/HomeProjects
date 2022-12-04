#include "stdafx.h"
#include <string>
#include <fstream>
#include <Windows.h>

#include "FileOperate.h"
#include "CommonUtility.h"


//===============================================================================================
// 
//===============================================================================================

std::string FileOperate::LoadStringFromArray(const std::vector<char>& buffer, int* indexToRead)
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

int FileOperate::LoadIntFromArray(const std::vector<char>& buffer, int* indexToRead)
{
	int number = 0;

	while (true)
	{
		number += buffer[*indexToRead] - '0';
		++(*indexToRead);
		if (!IsDigit(buffer[*indexToRead]))
			return number;
		number *= 10;
	}
	return 0;
}

//===============================================================================================
// 
//===============================================================================================

void FileOperate::LoadFromFile(const char* fullFileName, std::vector<WordsManager::WordInfo>& wordsInfo)
{
	std::ifstream file(fullFileName, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	if (size == -1)
		ExitMsg("Error opening file %s\n", fullFileName);
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer((int)size + 1);
	buffer[(int)size] = 0;
	if (!file.read(buffer.data(), size))
		ExitMsg("Error reading file %s\n", fullFileName);

	int parseIndex = 0;


	// Читаем основной блок слов
	while (true)
	{
		WordsManager::WordInfo wi;

		wi.word = LoadStringFromArray(buffer, &parseIndex);
		if (wi.word.length() == 0)         // При поиске начала строки был встречен конец файла (например, файл заканчивается пустой строкой)
			return;
		wi.translation = LoadStringFromArray(buffer, &parseIndex);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !IsDigit(buffer[parseIndex]))
			++parseIndex;

		if (IsDigit(buffer[parseIndex]))  // Читаем параметры
		{
			wi.checkOrderN = LoadIntFromArray(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !IsDigit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				ExitMsg("Sintax error in word %s", wi.word.c_str());
			// ---------
			wi.successCheckDays = LoadIntFromArray(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !IsDigit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				ExitMsg("Sintax error in word %s", wi.word.c_str());
			// ---------
			wi.lastDaySuccCheckTimestamp = LoadIntFromArray(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
				++parseIndex;
		}

		// Занести WordInfo в вектор
		wordsInfo.push_back(wi);

		if (buffer[parseIndex] == 0)
			return;
	}
}

//===============================================================================================
// 
//===============================================================================================

void FileOperate::SaveToFile(const char* fullFileName, const std::vector<WordsManager::WordInfo>& wordsInfo)
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
		ExitMsg("Can't create file %s\n", fullFileName);

	for (const auto& e : wordsInfo)
	{
		if (e.successCheckDays == 0)
			fprintf(f, "\"%s\" \"%s\"\n", e.word.c_str(), e.translation.c_str());
		else
		{
			fprintf(f, "\"%s\" \"%s\" %d %d %d",
				e.word.c_str(),
				e.translation.c_str(),
				e.checkOrderN,
				e.successCheckDays,
				e.lastDaySuccCheckTimestamp);

			fprintf(f, "\n");
		}
	}
	fclose(f);
}

