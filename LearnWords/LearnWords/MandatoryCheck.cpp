#include "stdafx.h"
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <chrono>

#include "LearnWordsApp.h"
#include "CommonUtility.h"
#include "AdditionalCheck.h"
#include "CloseTranslationWordsManager.h"
#include "MandatoryCheck.h"

extern Log logger;

//===============================================================================================
// 
//===============================================================================================

void MandatoryCheck::mandatory_check(time_t freezedTime, AdditionalCheck* pAdditionalCheck, const std::string& fullFileName)
{
	_learnWordsApp->clear_forgotten();
	clear_console_screen();

	// Выбрать слова для проверки

	std::vector<WordToCheck> wordsToRepeat;
	_learnWordsApp->collect_words_to_mandatory_check(wordsToRepeat, freezedTime);

	// Главный цикл проверки слов

	for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
	{
		WordsData::WordInfo& w = _pWordsData->_words[wordsToRepeat[i]._index];

		clear_console_screen();
		printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
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
		bool isQuickAnswer = _learnWordsApp->is_quick_answer(durationForAnswer, w.translation.c_str(), &ifTooLongAnswer);

		while (true)
		{
			clear_console_screen();
			printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
			_learnWordsApp->print_buttons_hints(w.translation, true);
			printf("\n  Осталось: %d, Быстрый ответ = %d, rightAnswersNum=%d\n", (int)wordsToRepeat.size() - i - 1, int(isQuickAnswer), w.rightAnswersNum);

			CloseTranslationWordsManager ctwm(_learnWordsApp, _pWordsData, wordsToRepeat[i]._index);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;                              //FIXME!!! внутри условий ниже много повторов!
			int keepPrevRightAnswersNum = w.rightAnswersNum;
			if (c == 72)  // Стрелка вверх
			{
				pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
				if (isQuickAnswer)
					w.isNeedSkipOneRandomLoop = true;
				_learnWordsApp->fill_dates_and_save(w, freezedTime, true, isQuickAnswer);
				if (ifTooLongAnswer)
					_learnWordsApp->_wordRememberedLong.insert(WordRememberedLong(wordsToRepeat[i]._index, durationForAnswer));
			}
			else
				if (c == 80) // Стрелка вниз
				{
					_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
					_learnWordsApp->set_as_forgotten(w);
					pAdditionalCheck->put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
					_learnWordsApp->fill_dates_and_save(w, freezedTime, false, false);
				}
				else
					if (c == 77) // Стрелка вправо (помним слово не очень уверенно)
					{
						_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
						_learnWordsApp->set_as_barely_known(w);
						pAdditionalCheck->put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
						_learnWordsApp->fill_dates_and_save(w, freezedTime, false, false);
					}
					else
						continue;
			logger("Check by time, word = %s, key=%d, PrevRightAnswersNum=%d, time = %s", w.word.c_str(), c, keepPrevRightAnswersNum, get_time_in_text(time(nullptr)));
			break;
		}
	}
}
