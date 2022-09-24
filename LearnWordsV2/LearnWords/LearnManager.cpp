#include "stdafx.h"
#include <vector>
#include <chrono>
#include <algorithm>

#include "Application.h"
#include "CommonUtility.h"
#include "LearnManager.h"

const int TIMES_TO_GUESS_TO_LEARNED = 3;  // Сколько раз нужно правильно назвать значение слова, чтобы оно считалось первично изученным
const int TIMES_TO_REPEAT_TO_LEARN  = 2;  // Сколько раз при изучении показать все слова сразу с переводом, прежде чем начать показывать без перевода
const int TIMES_TO_SHOW_A_WORD      = 5;  // Сколько шагов открытия каждого слова при первичном обучении

extern Log logger;

//===============================================================================================
// 
//===============================================================================================

void LearnManager::PrintMaskedTranslation(const char* _str, int symbolsToShowNum)
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
		if (IsSymbol(*str) && isInTranscription == false)
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

void LearnManager::PutToQueue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, bool needRandomInsert)
{
	int pos = (int)queue.size();

	if (needRandomInsert && (pos > 0 && RandFloat(0, 1) < 0.2f))
		--pos;

	queue.insert(queue.begin() + pos, wordToPut);
}

//===============================================================================================
// 
//===============================================================================================

bool LearnManager::AreAllWordsLearned(std::vector<WordToLearn>& queue)
{
	for (const auto& word : queue)
		if (word._localRightAnswersNum < TIMES_TO_GUESS_TO_LEARNED)
			return false;
	return true;
}

//===============================================================================================
// 
//===============================================================================================

void LearnManager::DoLearn(bool isLearnForgotten)
{
	auto wordsMgr = _pWordsData.lock();
	std::vector<int> wordsToLearnIds;

	// Составим список индексов слов, которые будем учить

	ClearConsoleScreen();

	if (!isLearnForgotten)  // Учим новые слова
	{
		printf("\nHow many words to learn: ");
		int wordsToLearn = EnterNumberFromConsole();
		if (wordsToLearn > 0)
			wordsToLearnIds = wordsMgr->GetUnlearnedTextsId(wordsToLearn);
		if (wordsToLearnIds.empty())
			return;
	}
	else  // Подучиваем слова забытые при проверке
	{
		wordsToLearnIds = wordsMgr->getForgottenList();
		wordsMgr->clearForgottenList();
	}

	// Первичное изучение (показываем все слова по одному разу)

	for (const auto& id : wordsToLearnIds)
	{
		ClearConsoleScreen();
		printf("\n%s\n\n", wordsMgr->GetWord(id).c_str());
		printf("%s", wordsMgr->GetTranslation(id).c_str());
		printf("\n");
		char c = 0;
		do
		{
			c = GetchFiltered();
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

				ClearConsoleScreen();
				printf("\n%s\n\n", wordsMgr->GetWord(id).c_str());
				PrintMaskedTranslation(wordsMgr->GetTranslation(id).c_str(), symbolsToShowNum);
				printf("\n\n  Arrow right - Open by one letter\n  Space -         Open the whole word");

				char c = 0;
				do
				{
					c = GetchFiltered();
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
		ClearConsoleScreen();

		// Выбрать слово, которое будем показывать
		WordToLearn wordToLearn;

		wordToLearn = learnCycleQueue[0];
		learnCycleQueue.erase(learnCycleQueue.begin());

		// Показываем слово
		int id = wordToLearn._index;
		printf("\n%s\n", wordsMgr->GetWord(id).c_str());
		char c = 0;
		do
		{
			c = GetchFiltered();
			if (c == 27)
				return;
		} while (c != ' ');
		wordsMgr->printTranslationDecorated(wordsMgr->GetTranslation(id));
		printf("\n\n  Arrow up  - I remember!\n  Arrow down   - I am not sure\n");

		// Обрабатываем ответ - знает ли пользователь слово
		while (true)
		{
			c = GetchFiltered();
			if (c == 27)  // ESC
				return;
			if (c == 72)  // Стрелка вверх
			{
				if (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED)
				{
					wordsMgr->SetWordAsJustLearned(id);
					wordsMgr->save();
					if (AreAllWordsLearned(learnCycleQueue))
						return;
				}
				PutToQueue(learnCycleQueue, wordToLearn, wordsToLearnIds.size()>3);
				break;
			}
			else
				if (c == 80) // Стрелка вниз
				{
					wordToLearn._localRightAnswersNum = 0;
					if (!isLearnForgotten)
						wordsMgr->SetWordAsUnlearned(id);
					wordsMgr->save();
					PutToQueue(learnCycleQueue, wordToLearn, wordsToLearnIds.size()>3);
					break;
				}
		}
	}
}

