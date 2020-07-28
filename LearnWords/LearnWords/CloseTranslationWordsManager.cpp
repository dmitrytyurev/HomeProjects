#include "stdafx.h"
#define NOMINMAX
#include <algorithm>

#include "CloseTranslationWordsManager.h"
#include "CommonUtility.h"
#include "WordsData.h"
#include "LearnWordsApp.h"

const int MIN_CLOSE_WORD_LEN = 5;                // Столько первых символов берётся из слов перевода, чтобы искать слова с похожими переводами

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::get_separate_words_from_translation(const char* __str, std::vector<std::string>& outWords)
{
	outWords.clear();
	const unsigned char* _str = reinterpret_cast<const unsigned char*>(__str);
	const unsigned char* str = _str;
	bool isInTranscription = false;
	bool isInRemark = false;
	bool isInWord = false;
	int  beginIndex = 0;
	int  endIndex = 0;
	while (*str)
	{
		if (*str == '[')
			isInTranscription = true;
		if (*str == ']')
			isInTranscription = false;
		if (*str == '(')
			isInRemark = true;
		if (*str == ')')
			isInRemark = false;
		if (is_russian_symbol(*str) && isInTranscription == false && isInRemark == false)
		{
			if (isInWord == false)
				beginIndex = str - _str;
			isInWord = true;
		}
		else
		{
			if (isInWord == true)
			{
				endIndex = str - _str;
				std::string s(__str + beginIndex, endIndex - beginIndex);
				outWords.push_back(s);
			}
			isInWord = false;
		}
		++str;
	}
	if (isInWord == true)
	{
		endIndex = str - _str;
		std::string s(__str + beginIndex, endIndex - beginIndex);
		outWords.push_back(s);
	}
}

//===============================================================================================
// 
//===============================================================================================

bool CloseTranslationWordsManager::is_word_in_filter_already(const std::string& word1, const std::string& word2)
{
	for (const auto& cep : _pWordsData->_compareExcludePairs)
	{
		if (cep.word1 == word2  &&  cep.word2 == word1)
			return true;
	}
	return false;
}


//===============================================================================================
// 
//===============================================================================================

bool CloseTranslationWordsManager::is_word_appended_to_translation_already(const std::string& engWord) const
{
	WordsData::WordInfo& wSrc = _pWordsData->_words[_srcWordIndex];

	if (wSrc.translation.find(engWord) != std::string::npos)
		return true;

	return false;
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::collect_close_words_to(std::vector<CloseWordFound>& closeWordsFound, const std::string& rusWord, int srcWordIndex, const std::string& engWord)
{
	int length = rusWord.length();
	if (length < MIN_CLOSE_WORD_LEN)
		return;
	int cutLen = std::max(MIN_CLOSE_WORD_LEN, length - 4);

	for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)
	{
		if (i == srcWordIndex)
			continue;
		WordsData::WordInfo& w = _pWordsData->_words[i];

		if (is_word_in_filter_already(w.word, engWord))
			continue;

		if (is_word_appended_to_translation_already(w.word))
			continue;

		std::vector<std::string> words;
		get_separate_words_from_translation(w.translation.c_str(), words);

		for (const auto& word : words)
		{
			if (word.compare(0, cutLen, rusWord, 0, cutLen) == 0)
			{
				CloseWordFound cwf;
				cwf.engWord = w.word;
				cwf.rusWordSrc = rusWord;
				cwf.translation = w.translation;
				closeWordsFound.push_back(cwf);
				break;
			}
		}
	}
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::print_close_words_by_translation()
{
	puts("\n\n\n\n");

	const WordsData::WordInfo& w = _pWordsData->_words[_srcWordIndex];

	std::vector<std::string> words;
	get_separate_words_from_translation(w.translation.c_str(), words);

	for (const auto& word : words)
		collect_close_words_to(_closeWordsFound, word, _srcWordIndex, w.word);

	for (int i = 0; i < (int)_closeWordsFound.size(); ++i)
		printf("%d. %s: %s\n", i + 1, _closeWordsFound[i].engWord.c_str(), _closeWordsFound[i].translation.c_str());
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::add_exclusion(int n)
{
	if (n >= (int)_closeWordsFound.size())
		return;
	const WordsData::WordInfo& w = _pWordsData->_words[_srcWordIndex];

	WordsData::CompareExcludePair cep;
	cep.word1 = w.word;
	cep.word2 = _closeWordsFound[n].engWord;
	_pWordsData->_compareExcludePairs.push_back(cep);
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::add_close_eng_word_to_translation(int n)
{
	if (n >= (int)_closeWordsFound.size())
		return;
	WordsData::WordInfo& w = _pWordsData->_words[_srcWordIndex];
	const std::string& wr = w.translation;
	// Найдём место, куда добавить похожее слово. Это место будет сразу после перевода, для которого мы нашли другое англ. слово с близким переводом

	size_t pos = wr.find(_closeWordsFound[n].rusWordSrc);

	if (pos == std::string::npos)
		exit_msg("Error 86036939");

	while (wr[pos] != 0 && wr[pos] != ',' && wr[pos] != '(' && wr[pos] != ';')
		++pos;

	if (wr[pos] == '(')   // Нужно добавить слово в список
	{
		while (wr[pos] != 0 && wr[pos] != ')')
			++pos;
		if (wr[pos] == 0)
			exit_msg("Sintax error 5725875");
		w.translation.insert(pos, ", " + _closeWordsFound[n].engWord);
	}
	else                 // Списка ещё нет, создадим его
		w.translation.insert(pos, " (" + _closeWordsFound[n].engWord + ")");
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::process_user_input(char c)
{
	char codesToAdd[] = { 49, 50, 51, 52, 53, 54, 55, 56, 57 };
	char codesToExclude[] = { 33, 64, 35, 36, 37, 94, 38, 42, 40 };

	const int sizeOfArr = sizeof(codesToAdd) / sizeof(codesToAdd[0]);

	for (int i = 0; i < sizeOfArr; ++i)
		if (codesToAdd[i] == c)
		{
			add_close_eng_word_to_translation(i);
			_learnWordsApp->save();
		}
		else
			if (codesToExclude[i] == c)
			{
				add_exclusion(i);
				_learnWordsApp->save();
			}
}
