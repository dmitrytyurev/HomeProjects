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
} quickAnswerTime[] = { {2100, 3500}, {2600, 4000}, {3100, 4500}, {3600, 5000}, {4100, 5500} }; // Время быстрого и долгого ответа в зависимости от числа переводов данного слова


extern Log logger;

//===============================================================================================
// 
//===============================================================================================

int CheckManager::get_word_id_to_check_impl(const std::vector<int>& lastCheckedIds)
{
	auto wordsMgr = _pWordsData.lock();
	const int TRESHOLD = 4; 
	// Группа 1: слова с successCheckDays <= TRESHOLD
	// Группа 2: слова с successCheckDays > TRESHOLD

	// Если одна из групп пуста, вернуть слово из второй группы с наименьшим checkOrderN
	int group1minOrderId = -1;
	int group1minOrder = 2000000000;
	int group2minOrderId = -1;
	int group2minOrder = 2000000000;
	for (int i=0; i< wordsMgr->GetWordsNum(); ++i)
	{
		const auto& w = wordsMgr->GetWordInfo(i);
		if (w.successCheckDays <= TRESHOLD)
		{
			if (w.checkOrderN < group1minOrder)
			{
				group1minOrder = w.checkOrderN;
				group1minOrderId = i;
			}
		}
		else
		{
			if (w.checkOrderN < group2minOrder)
			{
				group2minOrder = w.checkOrderN;
				group2minOrderId = i;
			}
		}
	}
	if (group1minOrderId == -1)
		return group2minOrderId;
	if (group2minOrderId == -1)
		return group1minOrderId;

	// Если ровно в одной из двух групп слово с наименьшим checkOrderN находится в списке из 10 последних повторённых, то вернуть слово из другого списка
	bool wasLastlyUsed1 = std::find(lastCheckedIds.begin(), lastCheckedIds.end(), group1minOrderId) != lastCheckedIds.end();
	bool wasLastlyUsed2 = std::find(lastCheckedIds.begin(), lastCheckedIds.end(), group2minOrderId) != lastCheckedIds.end();
	if (wasLastlyUsed1 != wasLastlyUsed2)
	{
		if (wasLastlyUsed1)
			return group2minOrderId;
		return group1minOrderId;
	}

	// Выдаём слова с чередованием из групп 1 и 2
	static bool switchFlag = false;
	switchFlag ^= true;
	if (switchFlag)
		return group1minOrderId;
	return group2minOrderId;
}

//===============================================================================================
// 
//===============================================================================================

int CheckManager::get_word_id_to_check(std::vector<int>& lastCheckedIds)
{
	auto wordsMgr = _pWordsData.lock();
	while(true)
	{
		int res = get_word_id_to_check_impl(lastCheckedIds);
		WordsManager::WordInfo& w = wordsMgr->GetWordInfo(res);
		if (!w.needSkip)
		{
			lastCheckedIds.push_back(res);
			if (lastCheckedIds.size() > 10)
				lastCheckedIds.erase(lastCheckedIds.begin());
			return res;
		}
		w.needSkip = false;
		wordsMgr->PutWordToEndOfQueue(res);
	}
	return 0; // Сюда не приходим
}

//===============================================================================================
// 
//===============================================================================================

void CheckManager::do_check()
{
	auto wordsMgr = _pWordsData.lock();
	std::vector<int> lastCheckedIds;   // Айдишники последних 10-ти проверенных слов

	clear_console_screen();

	printf("\nHow many words to check: ");
	int wordsToCheck = enter_number_from_console();
	if (wordsToCheck <= 0 || wordsMgr->GetWordsNum() == 0)
		return;

	// Главный цикл проверки слов
	for (int i = 0; i < wordsToCheck; ++i)
	{
		int id = get_word_id_to_check(lastCheckedIds);
		WordsManager::WordInfo& w = wordsMgr->GetWordInfo(id);

		clear_console_screen();
		printf("\n\n===============================\n %s\n===============================\n", wordsMgr->GetWord(id).c_str());
		auto t_start = std::chrono::high_resolution_clock::now();

		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');

		auto t_end = std::chrono::high_resolution_clock::now();
		double durationForAnswer = std::chrono::duration<double, std::milli>(t_end - t_start).count();
		bool ifTooLongAnswer = false;
		bool isQuickAnswer = is_quick_answer(durationForAnswer, wordsMgr->GetTranslation(id).c_str(), &ifTooLongAnswer);

		while (true)
		{
			clear_console_screen();
			printf("\n\n===============================\n %s\n===============================\n", wordsMgr->GetWord(id).c_str());
			wordsMgr->printTranslationDecorated(wordsMgr->GetTranslation(id));
			printf("\n\n  Arrow up  - I remember!\n  Arrow down   - I am not sure\n");
			printf("\n  Remain: %d, Quick = %d\n", wordsToCheck - i - 1, int(isQuickAnswer));

			c = getch_filtered();
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх
			{
				wordsMgr->PutWordToEndOfQueue(id);
				w.needSkip = isQuickAnswer;
				int curTimestamp = (int)std::time(nullptr);
				if (curTimestamp - w.lastDaySuccCheckTimestamp > 3600*24) {
					w.lastDaySuccCheckTimestamp = curTimestamp;
					w.successCheckDays++;
				}
				wordsMgr->save();
			}
			else
				if (c == 80) // Стрелка вниз
				{
					wordsMgr->addElementToForgottenList(id);
					wordsMgr->SetWordAsJustLearned(id);
					wordsMgr->save();
				}
				else
					continue;
			logger("Check by time, word = %s, key=%d, time = %s", wordsMgr->GetWord(id).c_str(), c, get_time_in_text(time(nullptr)));
			break;
		}
	}
}
//===============================================================================================
//
//===============================================================================================

bool CheckManager::is_quick_answer(double milliSec, const char* translation, bool* ifTooLongAnswer, double* extraDurationForAnswer)
{
	auto wordsMgr = _pWordsData.lock();
	int index = wordsMgr->getTranslationsNum(translation) - 1;
	const int timesNum = sizeof(quickAnswerTime) / sizeof(quickAnswerTime[0]);
	clamp_minmax(&index, 0, timesNum - 1);

	if (ifTooLongAnswer)
		*ifTooLongAnswer = milliSec > quickAnswerTime[index].max;

	if (extraDurationForAnswer)
		*extraDurationForAnswer = milliSec - (double)quickAnswerTime[index].min;

	return milliSec < quickAnswerTime[index].min;
}
