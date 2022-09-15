#include "stdafx.h"
#include "WordsData.h"
#include <string>
#include <vector>
#include <ctime>

std::vector<int> WordsData::GetUnlearnedTextsId(int textsNumNeeded)
{
	std::vector<int> result;
	for (int i = 0; i < (int)_words.size(); ++i)
	{
		if (_words[i].successCheckDays == 0)
		{
			result.push_back(i);
			if (--textsNumNeeded == 0)
				break;
		}
	}
	return result;
}

std::string WordsData::GetWord(int id)
{
	return _words[id].word;
}

std::string WordsData::GetTranslation(int id)
{
	return _words[id].translation;
}

void WordsData::SetTextAsJustLearned(int id)
{
	PutTextToEndOfQueue(id);
	_words[id].successCheckDays = 1;
	_words[id].lastDaySuccCheckTimestamp = (int)std::time(nullptr);
	_words[id].needSkip = false;
}

void WordsData::SetTextAsUnlearned(int id)
{
	_words[id].checkOrderN = 0;
	_words[id].successCheckDays = 0;
	_words[id].lastDaySuccCheckTimestamp = 0;
	_words[id].needSkip = false;
}

void WordsData::PutTextToEndOfQueue(int id)
{
	int maxOrderN = 0;
	for (const auto& word: _words)
	{
		maxOrderN = std::max(maxOrderN, word.checkOrderN);
	}
	_words[id].checkOrderN = maxOrderN + 1;
}

WordsData::WordInfo& WordsData::GetWordInfo(int id)
{
	return _words[id];
}
