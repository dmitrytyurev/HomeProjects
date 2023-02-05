#include "stdafx.h"
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <chrono>

#include "CommonUtility.h"
#include "CheckManager.h"
#include "WordsManager.h"

struct
{
	int min;
	int max;
} quickAnswerTime[] = { {2100 * 7 / 10, 3500}, {2600 * 7 / 10, 4000}, {3100 * 7 / 10, 4500}, {3600 * 7 / 10, 5000}, {4100 * 7 / 10, 5500} }; // Время быстрого и долгого ответа в зависимости от числа переводов данного слова


extern Log logger;

//===============================================================================================
// 
//===============================================================================================

int CheckManager::GetWordIdToCheck()
{
	auto wordsMgr = _pWordsData.lock();

	int minOrderId = -1;
	int minOrder = 2000000000;
	for (int i=0; i< wordsMgr->GetWordsNum(); ++i)
	{
		const auto& w = wordsMgr->GetWordInfo(i);
		if (w.successCheckDays > 0 && w.checkOrderN < minOrder)
		{
			minOrder = w.checkOrderN;
			minOrderId = i;
		}
	}
	
	return minOrderId;
}

//===============================================================================================
// 
//===============================================================================================

void CheckManager::DoCheck()
{
	auto wordsMgr = _pWordsData.lock();

	ClearConsoleScreen();

	printf("\nHow many words to check: ");
	int wordsToCheck = EnterNumberFromConsole();
	if (wordsToCheck <= 0 || wordsMgr->GetWordsNum() == 0)
		return;

	// Главный цикл проверки слов
	for (int i = 0; i < wordsToCheck; ++i)
	{
		int id = GetWordIdToCheck();
		WordsManager::WordInfo& w = wordsMgr->GetWordInfo(id);

		ClearConsoleScreen();
		printf("\n\n===============================\n %s\n===============================\n", wordsMgr->GetWord(id).c_str());
		auto t_start = std::chrono::high_resolution_clock::now();

		char c = 0;
		do
		{
			c = GetchFiltered();
			if (c == 27)
				return;
		} while (c != ' ');

		auto t_end = std::chrono::high_resolution_clock::now();
		double durationForAnswer = std::chrono::duration<double, std::milli>(t_end - t_start).count();
		bool ifTooLongAnswer = false;
		bool isQuickAnswer = IsQuickAnswer(durationForAnswer, wordsMgr->GetTranslation(id).c_str(), &ifTooLongAnswer);

		while (true)
		{
			ClearConsoleScreen();
			printf("\n\n===============================\n %s\n===============================\n", wordsMgr->GetWord(id).c_str());
			wordsMgr->printTranslationDecorated(wordsMgr->GetTranslation(id));
			printf("\n\n  Arrow up  - I remember!\n  Arrow down   - I am not sure\n");
			printf("\n  Remain: %d, Quick = %d\n", wordsToCheck - i - 1, int(isQuickAnswer));

			c = GetchFiltered();
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх
			{
				int curTimestamp = (int)std::time(nullptr);
				if (curTimestamp - w.lastDaySuccCheckTimestamp > 3600*24) {
					w.lastDaySuccCheckTimestamp = curTimestamp;
					w.successCheckDays++;
				}
				wordsMgr->PutWordToEndOfQueue(id, isQuickAnswer);
				wordsMgr->save();
				if (!isQuickAnswer) {
					wordsMgr->addElementToNotQuickList(id);
				}
			}
			else
				if (c == 80) // Стрелка вниз
				{
					wordsMgr->addElementToNotQuickList(id);
					wordsMgr->addElementToForgottenList(id);
					wordsMgr->SetWordAsJustLearned(id);
					wordsMgr->save();
				}
				else
					continue;
			logger("%s word = %s, key=%d, Quick=%d\n", GetTimeInText(time(nullptr)), wordsMgr->GetWord(id).c_str(), c, int(isQuickAnswer));
			break;
		}
	}
}
//===============================================================================================
//
//===============================================================================================

bool CheckManager::IsQuickAnswer(double milliSec, const char* translation, bool* ifTooLongAnswer, double* extraDurationForAnswer)
{
	auto wordsMgr = _pWordsData.lock();
	int index = wordsMgr->getTranslationsNum(translation) - 1;
	const int timesNum = sizeof(quickAnswerTime) / sizeof(quickAnswerTime[0]);
	ClampMinmax(&index, 0, timesNum - 1);

	if (ifTooLongAnswer)
		*ifTooLongAnswer = milliSec > quickAnswerTime[index].max;

	if (extraDurationForAnswer)
		*extraDurationForAnswer = milliSec - (double)quickAnswerTime[index].min;

	return milliSec < quickAnswerTime[index].min;
}
