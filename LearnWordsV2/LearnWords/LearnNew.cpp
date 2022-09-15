#include "stdafx.h"
#include <vector>
#include <chrono>
#include <algorithm>

#include "LearnWordsApp.h"
#include "CommonUtility.h"
#include "LearnNew.h"

const int TIMES_TO_GUESS_TO_LEARNED = 3;  // Сколько раз нужно правильно назвать значение слова, чтобы оно считалось первично изученным
const int TIMES_TO_REPEAT_TO_LEARN  = 2;  // Сколько раз при изучении показать все слова сразу с переводом, прежде чем начать показывать без перевода
const int TIMES_TO_SHOW_A_WORD      = 5;  // Сколько шагов открытия каждого слова при первичном обучении

extern Log logger;

//===============================================================================================
// 
//===============================================================================================

void LearnNew::print_masked_translation(const char* _str, int symbolsToShowNum)
{
	const unsigned char* str = reinterpret_cast<const unsigned char*>(_str);;
	bool isInTranscription = false;
	int  charCounterInWord = 0;
	while (*str)
	{
		if (*str == '[')
			isInTranscription = true;
		if (*str == ']')
			isInTranscription = false;
		bool isInWord = false;
		if (is_symbol(*str) && isInTranscription == false)
		{
			isInWord = true;
			++charCounterInWord;
		}
		else
			charCounterInWord = 0;

		if (isInWord && (charCounterInWord > symbolsToShowNum))
			printf("_");
		else
			printf("%c", *str);
		++str;
	}
}

//===============================================================================================
// Вставляем рандомно в последнюю или предпоследнюю позицию 
//===============================================================================================

void LearnNew::put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, bool needRandomInsert)
{
	int pos = (int)queue.size();

	if (needRandomInsert && (pos > 0 && rand_float(0, 1) < 0.2f))
		--pos;

	queue.insert(queue.begin() + pos, wordToPut);
}

//===============================================================================================
// 
//===============================================================================================

bool LearnNew::are_all_words_learned(std::vector<WordToLearn>& queue)
{
	for (const auto& word : queue)
		if (word._localRightAnswersNum < TIMES_TO_GUESS_TO_LEARNED)
			return false;
	return true;
}

//===============================================================================================
// 
//===============================================================================================

void LearnNew::learn_new()
{
	std::vector<int> wordsToLearnIds;

	// Составим список индексов слов, которые будем учить

	clear_console_screen();
	printf("\nHow many words to learn: ");
	int wordsToLearn = enter_number_from_console();
	if (wordsToLearn > 0)
		wordsToLearnIds = _pWordsData->GetUnlearnedTextsId(wordsToLearn);
	if (wordsToLearnIds.empty())
		return;

	// Первичное изучение (показываем все слова по одному разу)

	for (const auto& id : wordsToLearnIds)
	{
		clear_console_screen();
		printf("\n%s\n\n", _pWordsData->GetWord(id).c_str());
		printf("%s", _pWordsData->GetTranslation(id).c_str());
		printf("\n");
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
	}

	// Первичное изучение (показываем неколько раз, сначала перевод скрыт, но можно открыть целиком или по буквам)

	for (int i2 = 0; i2 < TIMES_TO_REPEAT_TO_LEARN; ++i2)
	{
		for (const auto& id : wordsToLearnIds)
		{
			for (int i3 = 0; i3 < TIMES_TO_SHOW_A_WORD; ++i3)
			{
				int symbolsToShowNum = (i3 != TIMES_TO_SHOW_A_WORD -1 ? i3 : 100);

				clear_console_screen();
				printf("\n%s\n\n", _pWordsData->GetWord(id).c_str());
				print_masked_translation(_pWordsData->GetTranslation(id).c_str(), symbolsToShowNum);
				printf("\n\n  Arrow right - Open by one letter\n  Space -         Open the whole word");

				char c = 0;
				do
				{
					c = getch_filtered();
					if (c == 27)
						return;
					if (c == ' ')  // Пробел - показать слово целиком
					{
						if (i3 < TIMES_TO_SHOW_A_WORD - 2)
							i3 = TIMES_TO_SHOW_A_WORD - 2;  // Сразу открыть слово, без плавного появления
						break;
					}
				} while (c != 77);  // Стрелка вправо - открывать по одному символу
			}
		}
	}

	// Второй этап - слова показываются без перевода. Если пользователь угадает значение более TIMES_TO_GUESS_TO_LEARNED раз,
	// то слово считается изученным. Цикл изучения заканчивается, когда все слова изучены.

	std::vector<WordToLearn> learnCycleQueue;  // Циклическая очередь слов в процессе изучения (берём из начала, добавляем в последнюю или предпоследнюю позицию)

											   // Занести слова, которые будем изучать в очередь
	for (const auto& id : wordsToLearnIds)
	{
		WordToLearn word(id);
		learnCycleQueue.push_back(word);
	}
																									 // Главный цикл обучения
	while (true)
	{
		clear_console_screen();

		// Выбрать слово, которое будем показывать
		WordToLearn wordToLearn;

		wordToLearn = learnCycleQueue[0];
		learnCycleQueue.erase(learnCycleQueue.begin());

		// Показываем слово
		int id = wordToLearn._index;
		printf("\n%s\n", _pWordsData->GetWord(id).c_str());
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
		_learnWordsApp->print_buttons_hints(_pWordsData->GetTranslation(id), false);

		// Обрабатываем ответ - знает ли пользователь слово
		while (true)
		{
			c = getch_filtered();
			if (c == 27)  // ESC
				return;
			if (c == 72)  // Стрелка вверх
			{
				if (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED)
				{
					_pWordsData->SetTextAsJustLearned(id);
					_learnWordsApp->save();
					if (are_all_words_learned(learnCycleQueue))
						return;
				}
				put_to_queue(learnCycleQueue, wordToLearn, true);
				break;
			}
			else
				if (c == 80) // Стрелка вниз
				{
					wordToLearn._localRightAnswersNum = 0;
					put_to_queue(learnCycleQueue, wordToLearn, true);
					_pWordsData->SetTextAsUnlearned(id);
					_learnWordsApp->save();
					break;
				}
		}
	}
}

