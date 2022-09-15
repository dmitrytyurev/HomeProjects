#include "stdafx.h"
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <chrono>

#include "LearnWordsApp.h"
#include "CommonUtility.h"
#include "Check.h"

extern Log logger;

//===============================================================================================
// 
//===============================================================================================

int Check::get_word_id_to_check_impl(const std::vector<int>& lastCheckedIds)
{
	const int TRESHOLD = 4; 
	// Группа 1: слова с successCheckDays <= TRESHOLD
	// Группа 2: слова с successCheckDays > TRESHOLD

	// Если одна из групп пуста, вернуть слово из второй группы с наименьшим checkOrderN
	int group1minOrderId = -1;
	int group1minOrder = 2000000000;
	int group2minOrderId = -1;
	int group2minOrder = 2000000000;
	for (int i=0; i<_pWordsData->_words.size(); ++i)
	{
		const auto& w = _pWordsData->_words[i];
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

int Check::get_word_id_to_check(std::vector<int>& lastCheckedIds)
{
	while(true)
	{
		int res = get_word_id_to_check_impl(lastCheckedIds);
		if (!_pWordsData->_words[res].needSkip)
		{
			lastCheckedIds.push_back(res);
			if (lastCheckedIds.size() > 10)
				lastCheckedIds.erase(lastCheckedIds.begin());
			return res;
		}
		_pWordsData->_words[res].needSkip = false;
		_pWordsData->PutTextToEndOfQueue(res);
	}
	return 0; // Сюда не приходим
}

//===============================================================================================
// 
//===============================================================================================

void Check::do_check()
{
	std::vector<int> lastCheckedIds;   // Айдишники последних 10-ти проверенных слов

	clear_console_screen();

	printf("\nHow many words to check: ");
	int wordsToCheck = enter_number_from_console();
	if (wordsToCheck <= 0 || _pWordsData->_words.empty())
		return;

	// Главный цикл проверки слов
	for (int i = 0; i < wordsToCheck; ++i)
	{
		int id = get_word_id_to_check(lastCheckedIds);
		WordsData::WordInfo& w = _pWordsData->GetWordInfo(id);

		clear_console_screen();
		printf("\n\n===============================\n %s\n===============================\n", _pWordsData->GetWord(id).c_str());
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
		bool isQuickAnswer = _learnWordsApp->is_quick_answer(durationForAnswer, _pWordsData->GetTranslation(id).c_str(), &ifTooLongAnswer);

		while (true)
		{
			clear_console_screen();
			printf("\n\n===============================\n %s\n===============================\n", _pWordsData->GetWord(id).c_str());
			_learnWordsApp->print_buttons_hints(_pWordsData->GetTranslation(id), true);
			printf("\n  Remain: %d, Quick = %d\n", wordsToCheck - i - 1, int(isQuickAnswer));

			c = getch_filtered();
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх
			{
				_pWordsData->PutTextToEndOfQueue(id);
				w.needSkip = isQuickAnswer;
				int curTimestamp = (int)std::time(nullptr);
				if (curTimestamp - w.lastDaySuccCheckTimestamp > 3600*24) {
					w.lastDaySuccCheckTimestamp = curTimestamp;
					w.successCheckDays++;
				}
				_learnWordsApp->save();
			}
			else
				if (c == 80) // Стрелка вниз
				{
					_learnWordsApp->_forgottenWordsIndices.push_back(id);
					_pWordsData->SetTextAsJustLearned(id);
					_learnWordsApp->save();
				}
				else
					continue;
			logger("Check by time, word = %s, key=%d, time = %s", _pWordsData->GetWord(id).c_str(), c, get_time_in_text(time(nullptr)));
			break;
		}
	}
}
