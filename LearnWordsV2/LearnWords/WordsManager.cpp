#include "stdafx.h"
#include "WordsManager.h"
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <windows.h>

#include "CommonUtility.h"
#include "FileOperate.h"

extern Log logger;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


WordsManager::WordsManager(const std::string& wordsFileName): _fullFileName(wordsFileName)
{
	load(wordsFileName);
}

void WordsManager::load(const std::string& wordsFileName)
{
	FileOperate::LoadFromFile(wordsFileName.c_str(), _words);
}

std::vector<int> WordsManager::GetUnlearnedTextsId(int textsNumNeeded)
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

std::string WordsManager::GetWord(int id)
{
	return _words[id].word;
}

std::string WordsManager::GetTranslation(int id)
{
	return _words[id].translation;
}

int WordsManager::GetWordsNum()
{
	return static_cast<int>(_words.size());
}

void WordsManager::SetWordAsJustLearned(int id)
{
	_words[id].lastDaySuccCheckTimestamp = (int)std::time(nullptr);
	_words[id].successCheckDays = 1;
	PutWordToEndOfQueue(id, 10000);
}

void WordsManager::SetWordAsUnlearned(int id)
{
	_words[id].checkOrderN = 0;
	_words[id].successCheckDays = 0;
	_words[id].lastDaySuccCheckTimestamp = 0;
}

void WordsManager::PutWordToEndOfQueue(int id, int timeToAnswer)
{
	int maxOrderN = 0;
	for (int i = 0; i < _words.size(); ++i)
	{
		if (i != id && isWordLearned(i))
		{
			maxOrderN = std::max(maxOrderN, _words[i].checkOrderN);
		}
	}

	int prevOrderN = _words[id].checkOrderN;

	_words[id].checkOrderN = maxOrderN + 1;
	int maxPrevTimeToAnswer = _maxAnswerTime.find(id) != _maxAnswerTime.end() ? _maxAnswerTime[id] : 0;
	_words[id].timeToAnswer = std::max(timeToAnswer, maxPrevTimeToAnswer);
}

WordsManager::WordInfo& WordsManager::GetWordInfo(int id)
{
	return _words[id];
}


int WordsManager::GetWordIdByOrder(int orderN)
{
	for (int i = 0; i < _words.size(); ++i)
	{
		if (_words[i].checkOrderN == orderN)
		{
			return i;
		}
	}
	return -1;
}

int WordsManager::GetWordIdByWord(const std::string& word)
{
	for (int i = 0; i < _words.size(); ++i)
	{
		if (_words[i].word == word)
		{
			return i;
		}
	}
	return -1;
}


//===============================================================================================
// Вернуть число переводов в данном слове
//===============================================================================================

int WordsManager::getTranslationsNum(const char* translation)
{
	const char* p = translation;
	int inBracesNestedCount = 0;
	int commasCount = 0;

	while (*p)
	{
		if (*p == '(')
			++inBracesNestedCount;
		else
			if (*p == ')')
				--inBracesNestedCount;
			else
				if (*p == ',' && inBracesNestedCount == 0)
					++commasCount;
		++p;
	}

	return commasCount + 1;
}

//===============================================================================================

bool WordsManager::isWordLearnedRecently(int id)
{
	const int TRESHOLD = 4;
	return  isWordLearned(id) && _words[id].successCheckDays <= TRESHOLD;
}

//===============================================================================================

bool WordsManager::isWordLearned(int id)
{
	return _words[id].successCheckDays > 0;
}

//===============================================================================================

void WordsManager::save()
{
	std::vector<WordInfo> wordsToSave = _words;

	std::sort(wordsToSave.begin(), wordsToSave.end());
	FileOperate::SaveToFile(_fullFileName.c_str(), wordsToSave);
}

//===============================================================================================
// Печатает перевод с переносом значений (через ,) и затемняя произношение ([])
//===============================================================================================

void WordsManager::printTranslationDecorated(const std::string& str)
{
	CONSOLE_SCREEN_BUFFER_INFO   csbi;
	csbi.wAttributes = 0;
	bool isColorReadSucsess = false;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole != INVALID_HANDLE_VALUE)
		isColorReadSucsess = GetConsoleScreenBufferInfo(hConsole, &csbi) == TRUE;

	const char* p = str.c_str();
	int bracesNestCount = 0;

	printf("\n");
	if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
		SetConsoleTextAttribute(hConsole, 15);
	if (str.find('[') == std::string::npos)
		printf(" ");

	while (*p)
	{
		if (*p == '(' || *p == '[')
		{
			++bracesNestCount;
			if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
				SetConsoleTextAttribute(hConsole, 8);
		}

		bool isCommaSeparatingMeanings = (*p == ',' && bracesNestCount == 0);
		if (!isCommaSeparatingMeanings)
			printf("%c", *p);

		if (*p == ')' || *p == ']')
		{
			--bracesNestCount;
			if (bracesNestCount == 0)
				if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
					SetConsoleTextAttribute(hConsole, 15);
		}

		if (*p == ']' || (*p == ',' && bracesNestCount == 0))
		{
			printf("\n");
			if (*p == ']')
				printf("\n");
		}

		++p;
	}
	printf("\n");

	if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
		SetConsoleTextAttribute(hConsole, csbi.wAttributes);
}

//===============================================================================================

void WordsManager::clearForgottenList()
{
	_forgottenWordsIndices.clear();
}

//===============================================================================================

bool WordsManager::isForgottenListEmpty()
{
	return _forgottenWordsIndices.empty();
}

//===============================================================================================

const std::vector<int>& WordsManager::getForgottenList()
{
	return _forgottenWordsIndices;
}

//===============================================================================================

void WordsManager::addElementToForgottenList(int id)
{
	if (std::find(std::begin(_forgottenWordsIndices), std::end(_forgottenWordsIndices), id) == std::end(_forgottenWordsIndices))
	{
		_forgottenWordsIndices.push_back(id);
	}
}

//===============================================================================================

void WordsManager::updateMaxTimeToAnswer(int id, int timeToAnswer)
{
	if (_maxAnswerTime.find(id) != _maxAnswerTime.end())
	{
		_maxAnswerTime[id] = std::max(timeToAnswer, _maxAnswerTime[id]);
	}
	else
	{
		_maxAnswerTime[id] = timeToAnswer;
	}
}
