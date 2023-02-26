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
	_words[id].successCheckDays = 0;   // Чтобы в методе PutWordToEndOfQueue это слово не использовалось для поиска свободного места
	PutWordToEndOfQueue(id, false);
	_words[id].successCheckDays = 1;
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

	int prevOrderN = _words[id].checkOrderN != 0 ? _words[id].checkOrderN : minOrderN - 1;

	// Найдём первое свободное значение checkOrderN больше prevOrderN
	int firstUnused = prevOrderN + 1;
	while (true)
	{
		bool found = false;
		for (const auto& word : _words)
		{
			if (word.successCheckDays > 0 && word.checkOrderN == firstUnused)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			break;
		}
		++firstUnused;
	}

	if (wasQuickAnswer && 
		!isWordLearnedRecently(id) && 
		std::find(std::begin(_notQuickWordsIndices), std::end(_notQuickWordsIndices), id) == std::end(_notQuickWordsIndices))
	{
		logger("Treated as quick: ");
		// Не просто в конец очереди, а сдвинуть на попозже (отложенные слова)
		int shiftAdd = std::max(1, std::min(50, int(learnedNum * 0.3)));  // На сколько слов хотим отложить проверку данного слова (его ведь хорошо помним)
		int tryOrderN = firstUnused + shiftAdd;  // Номер, который хотим назначить
		if (tryOrderN > maxOrderN)
		{
			_words[id].checkOrderN = tryOrderN;
		}
		else
		{
			_words[id].checkOrderN = maxOrderN + 1;
		}
	}
	else
	{
		logger("Treated as not quick: ");
		if (!isWordLearnedRecently(id))
		{
			// В конец очереди неотложенных слов (отложенные идут чуть дальше, за них не залезаем)
			_words[id].checkOrderN = firstUnused;
		}
		else
		{
			const int LEARNED_RECENTLY_SHIFT = 22;  
			// Не очень далеко поместить:
			// Если ближайшая свободная (firstUnused) находится не дальше LEARNED_RECENTLY_SHIFT, то вставляем туда
			// Если дальше, то попробуем вставить на позицию: старая + LEARNED_RECENTLY_SHIFT
			// Но не раньше, чем последнее недавновыученное слово + 1
			if (firstUnused <= prevOrderN + LEARNED_RECENTLY_SHIFT)
			{
				_words[id].checkOrderN = firstUnused;
			}
			else
			{
				int keepPrev = firstUnused;
				int checkOrderN = firstUnused - 1;
				while (true)
				{
					int id = GetWordIdByOrder(checkOrderN);
					if (id == -1)
					{
						ExitMsg("Word not found by order");
					}
					if (isWordLearnedRecently(id))
					{
						checkOrderN = keepPrev;
						break;
					}
					if (checkOrderN < prevOrderN + LEARNED_RECENTLY_SHIFT)
					{
						break;
					}
					keepPrev = checkOrderN;
					--checkOrderN;
				}
				// Если присваиваем не пустой индекс а уже использованный, то сдвинем меньшие индексы вниз
				if (checkOrderN != firstUnused)
				{
					for (int i = 0; i < _words.size(); ++i)
					{
						if (_words[i].successCheckDays > 0) // Слово изучено
						{
							if (_words[i].checkOrderN <= checkOrderN)
							{
								--(_words[i].checkOrderN);
							}
						}
					}
				}
				_words[id].checkOrderN = checkOrderN;
			}
		}
	}
	// Если мы перезаписали слово не с меньшим исходным индексом, значит образовалась дырка в нумерации, откуда забрали индекс.
	// Сдвинем идексы ниже, чтобы убрать дырку. Мы позволяем образовываться дыркам только при присвоении индекса быстро изученным словам.
	// Но не позволяем образовываться дыркам, при забирании неминимального индекса. Забрать же неминимальный индекс можем, поскольку при выборе слова для изучения
	// рассматриваем слова не как один список, а как два списка - давно изученных и недавно изученных.

	for (int i = 0; i < _words.size(); ++i)
	{
		if (_words[i].successCheckDays > 0) // Слово изучено
		{
			if (_words[i].checkOrderN < prevOrderN)
			{
				++(_words[i].checkOrderN);
			}
		}
	}
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
	return _words[id].successCheckDays <= TRESHOLD;
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
	_forgottenWordsIndices.push_back(id);
}

//===============================================================================================

void WordsManager::addElementToNotQuickList(int id)
{
	_notQuickWordsIndices.push_back(id);
}
