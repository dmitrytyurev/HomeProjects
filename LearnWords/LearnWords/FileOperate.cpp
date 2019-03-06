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

	// Читаем сохранённый позиции в аудировании
	while (true)
	{
		WordsData::ListeningTextToKeep lttk;

		lttk.rimFolder = load_string_from_array(buffer, &parseIndex);
		if (lttk.rimFolder.length() == 0)         // При поиске начала строки был встречен конец файла (например, файл заканчивается пустой строкой)
			exit_msg("Sintax error in word %s", lttk.rimFolder.c_str());

		if (lttk.rimFolder == "Compare exclude pairs")
			break;

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
			++parseIndex;
		if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
			exit_msg("Sintax error in parameter %s", lttk.rimFolder.c_str());
		lttk.volumeN = load_int_from_array(buffer, &parseIndex);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
			++parseIndex;
		if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
			exit_msg("Sintax error in parameter %s", lttk.rimFolder.c_str());		lttk.phraseN = load_int_from_array(buffer, &parseIndex);

		// Занести ListeningTextToKeep в вектор
		pWordsData->_listeningTextsToKeep.push_back(lttk);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
			++parseIndex;

		if (buffer[parseIndex] == 0)
			exit_msg("Sintax error N96309892");
	}

	// Читаем пары исключений
	while (true)
	{
		WordsData::CompareExcludePair cep;

		cep.word1 = load_string_from_array(buffer, &parseIndex);
		if (cep.word1.length() == 0)         // При поиске начала строки был встречен конец файла (например, файл заканчивается пустой строкой)
			exit_msg("Sintax error in parameter %s", cep.word1.c_str());

		if (cep.word1 == "Main block")
			break;

		cep.word2 = load_string_from_array(buffer, &parseIndex);

		// Занести CompareExcludePair в вектор
		pWordsData->_compareExcludePairs.push_back(cep);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
			++parseIndex;

		if (buffer[parseIndex] == 0)
			exit_msg("Sintax error N86093486");
	}

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
			wi.rightAnswersNum = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());
			// ---------
			wi.dateOfRepeat = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());
			// ---------
			wi.randomTestIncID = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());

			// ---------
			wi.cantRandomTestedAfter = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());

			// ---------
			wi.isInFastRandomQueue = !!load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());

			// ---------
			wi.isNeedSkipOneRandomLoop = !!load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());

			// ---------
			wi.cantRandomTestedBefore = load_int_from_array(buffer, &parseIndex);
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

	for (const auto& e : pWordsData->_listeningTextsToKeep)
	{
		fprintf(f, "\"%s\" %d %d\n",
			e.rimFolder.c_str(),
			e.volumeN,
			e.phraseN);
	}

	fprintf(f, "\"Compare exclude pairs\"\n");

	for (const auto& e : pWordsData->_compareExcludePairs)
	{
		fprintf(f, "\"%s\" \"%s\"\n",
			e.word1.c_str(),
			e.word2.c_str());
	}

	fprintf(f, "\"Main block\"\n");

	for (const auto& e : pWordsData->_words)
	{
		if (e.rightAnswersNum == 0)
			fprintf(f, "\"%s\" \"%s\"\n", e.word.c_str(), e.translation.c_str());
		else
		{
			fprintf(f, "\"%s\" \"%s\" %d %d %d %d %d %d %d",
				e.word.c_str(),
				e.translation.c_str(),
				e.rightAnswersNum,
				e.dateOfRepeat,
				e.randomTestIncID,
				e.cantRandomTestedAfter,
				int(e.isInFastRandomQueue),
				int(e.isNeedSkipOneRandomLoop),
				e.cantRandomTestedBefore);

			fprintf(f, "\n");
		}
	}
	fclose(f);
}

//===============================================================================================
// 
//===============================================================================================

void FileOperate::export_for_google_doc(WordsData* pWordsData)
{
	const char* fullNameForExpot = "c:\\eng_learn_export.txt";

	FILE* f = nullptr;
	fopen_s(&f, fullNameForExpot, "wt");
	if (f == nullptr)
		exit_msg("Can't create file %s\n", fullNameForExpot);

	for (const auto& e : pWordsData->_words)
	{
		if (e.rightAnswersNum == 0)
			fprintf(f, "%s#%s\n", e.word.c_str(), e.translation.c_str());
		else
		{
			fprintf(f, "%s#%s#%d#%d#%d#%d#%d#%d#%d",
				e.word.c_str(),
				e.translation.c_str(),
				e.rightAnswersNum,
				e.dateOfRepeat,
				e.randomTestIncID,
				e.cantRandomTestedAfter,
				int(e.isInFastRandomQueue),
				int(e.isNeedSkipOneRandomLoop),
				e.cantRandomTestedBefore);

			fprintf(f, "\n");
		}
	}
	fclose(f);
}
