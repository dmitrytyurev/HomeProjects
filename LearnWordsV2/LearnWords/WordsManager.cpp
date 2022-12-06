#include "stdafx.h"
#include "WordsManager.h"
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <windows.h>
#include "FileOperate.h"

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
	PutWordToEndOfQueue(id, false);
	_words[id].successCheckDays = 1;
	_words[id].lastDaySuccCheckTimestamp = (int)std::time(nullptr);
}

void WordsManager::SetWordAsUnlearned(int id)
{
	_words[id].checkOrderN = 0;
	_words[id].successCheckDays = 0;
	_words[id].lastDaySuccCheckTimestamp = 0;
}

void WordsManager::PutWordToEndOfQueue(int id, bool wasQuickAnswer)
{
	int minOrderN = INT_MAX;
	int maxOrderN = 0;
	int learnedNum = 0;
	for (const auto& word: _words)
	{
		if (word.successCheckDays > 0) // Слово изучено
		{
			++learnedNum;
			minOrderN = std::min(minOrderN, word.checkOrderN);
			maxOrderN = std::max(maxOrderN, word.checkOrderN);
		}
	}
	if (minOrderN == INT_MAX)
	{
		minOrderN = 0;
	}

	int prevOrderN = _words[id].checkOrderN;
	if (wasQuickAnswer)
	{
		int spaceNum = maxOrderN - minOrderN + 1 - learnedNum;  // Количество дырок в нумерации
		int add = std::max(1, (100-spaceNum)/4);
		_words[id].checkOrderN = maxOrderN + add;
	}
	else
	{
		// Используем первое свободное значение checkOrderN больше minOrderN
		int n = _words[id].checkOrderN + 1;
		while(true)
		{
			bool found = false;
			for (const auto& word : _words)
			{
				if (word.successCheckDays > 0 && word.checkOrderN == n)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				_words[id].checkOrderN = n;
				break;
			}
			++n;
		}
	}
	// Если мы перезаписали слово не с меньшим индексом, значит образовалась дырка в нумерации, откуда забрали индекс.
	// Сдвинем идексы ниже, чтобы убрать дырку. Мы позволяем образовываться дыркам только при присвоении индекса быстро изученным словам.
	// Но не позволяем образовываться дыркам, при забирании неминимального индекса. Забрать неминимальный индекс можем, поскольку при выборе слова для изучения
	// рассматриваем слова не как один список, а как два списка - давно изученных и недавно изученных.
	while (true)
	{
		// Найти слово с индексом меньше нашего прошлого, причём ближайшее
		int indexFound = -1;
		int minDist = INT_MAX;
		for (int i = 0; i < _words.size(); ++i)
		{
			if (_words[i].successCheckDays > 0) // Слово изучено
			{
				if (_words[i].checkOrderN < prevOrderN && prevOrderN - _words[i].checkOrderN < minDist)
				{
					indexFound = i;
					minDist = prevOrderN - _words[i].checkOrderN;
				}
			}
		}
		if (indexFound == -1)
		{
			break;
		}
		int tmp = _words[indexFound].checkOrderN;
		_words[indexFound].checkOrderN = prevOrderN;
		prevOrderN = tmp;
	}
}

WordsManager::WordInfo& WordsManager::GetWordInfo(int id)
{
	return _words[id];
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

void WordsManager::save()
{
	FileOperate::SaveToFile(_fullFileName.c_str(), _words);
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
	_forgottenWordsIndices.push_back(id);
}
