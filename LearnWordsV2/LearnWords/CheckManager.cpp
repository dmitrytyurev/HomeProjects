#include "stdafx.h"
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <chrono>

#include "CommonUtility.h"
#include "CheckManager.h"
#include "WordsManager.h"

int quickAnswerTime[] = { 1400, 1900, 2500 }; // Время быстрого ответа


extern Log logger;

const int COEFF1 = 800; // Количество миллисекунд в течение которых полностью растрачивается преимущество по быстрому времени ответа
const int COEFF2 = 120;  // Максимальный сдвиг вперёд в очереди для слов, на которые ответил быстро

struct ConfuseWords
{
	std::string word1;
	std::string word2;
};
std::vector<ConfuseWords> confuseWords = { {"Plummet", "Plunge"}, {"Discern", "Diverge"} };


//===============================================================================================
// 
//===============================================================================================

int CheckManager::GetWordIdToCheck(std::unique_ptr<WordsManager>& wordsMgr, double prob, double& probSub, std::vector<int>& learnedRecentlyIds)
{
	// Если предыдущее слово было одним из пары, которые мы путали раньше, то выдаём теперь второе слово из этой пары
	static std::string prevWord;
	static std::string prevPrevWord;

	int minOrderId = -1;

	for (auto w : confuseWords)
	{
		if (w.word1 == prevWord && prevPrevWord != w.word2)
		{
			minOrderId = wordsMgr->GetWordIdByWord(w.word2);
			break;
		}
		if (w.word2 == prevWord && prevPrevWord != w.word1)
		{
			minOrderId = wordsMgr->GetWordIdByWord(w.word1);
			break;
		}
	}

	// Выбираем слово из недавно изученных c переданной вероятностью
	if (minOrderId == -1 && !learnedRecentlyIds.empty())
	{
		probSub -= 1. / (prob + 1.);
		if (probSub < 0) {
			probSub += 1.;
			int minOrder = INT_MAX;

			for (auto id: learnedRecentlyIds)
			{
				const auto& w = wordsMgr->GetWordInfo(id);
				if (w.checkOrderN < minOrder)
				{
					minOrder = w.checkOrderN;
					minOrderId = id;
				}
			}
		}
	}

	// Выбираем слово из остальных. К словам на которые быстро отвечали, добавляем коэффициент
	if (minOrderId == -1)
	{
		int minOrder = INT_MAX;
		for (int i = 0; i < wordsMgr->GetWordsNum(); ++i)
		{
			const auto& w = wordsMgr->GetWordInfo(i);

			int shift = COEFF2 - COEFF2 * (w.timeToAnswer - quickAnswerTime[0]) / COEFF1;
			ClampMinmax(&shift, 0, COEFF2);

			if (!wordsMgr->isWordLearnedRecently(i) && wordsMgr->isWordLearned(i) && w.checkOrderN + shift < minOrder)
			{
				minOrder = w.checkOrderN + shift;
				minOrderId = i;
			}
		}
	}

	// Если выбранное слово состоит в паре слов, которые мы путали раньше, то рандомно можем выбрать второе слово из пары,
	// чтобы слова в паре не шли всегда в одном порядке

	for (auto w : confuseWords)
	{
		if (w.word1 == wordsMgr->GetWordInfo(minOrderId).word && w.word2 != prevWord && RandFloat(0.f, 1.f) < 0.5f)
		{
			minOrderId = wordsMgr->GetWordIdByWord(w.word2);
			break;
		}
		if (w.word2 == wordsMgr->GetWordInfo(minOrderId).word && w.word1 != prevWord && RandFloat(0.f, 1.f) < 0.5f)
		{
			minOrderId = wordsMgr->GetWordIdByWord(w.word1);
			break;
		}
	}

	prevPrevWord = prevWord;
	prevWord = wordsMgr->GetWordInfo(minOrderId).word;
	return minOrderId;
}

//===============================================================================================
// 
//===============================================================================================

void CheckManager::ProcessRemember(std::unique_ptr<WordsManager>& wordsMgr, int id, int timeToAnswer)
{
	WordsManager::WordInfo& w = wordsMgr->GetWordInfo(id);
	int curTimestamp = (int)std::time(nullptr);
	if (curTimestamp - w.lastDaySuccCheckTimestamp > 3600*24) {
		w.lastDaySuccCheckTimestamp = curTimestamp;
		w.successCheckDays++;
	}
	wordsMgr->PutWordToEndOfQueue(id, timeToAnswer);
	wordsMgr->save();
	wordsMgr->updateMaxTimeToAnswer(id, timeToAnswer);
}

void CheckManager::ProcessForget(std::unique_ptr<WordsManager>& wordsMgr, int id)
{
	wordsMgr->updateMaxTimeToAnswer(id, 10000);
	wordsMgr->addElementToForgottenList(id);
	wordsMgr->SetWordAsJustLearned(id);
	wordsMgr->save();
}

void CheckManager::DoCheck(std::unique_ptr<WordsManager>& wordsMgr)
{
	ClearConsoleScreen();

	printf("\nHow many words to check: ");
	int wordsToCheck = EnterNumberFromConsole();
	if (wordsToCheck <= 0 || wordsMgr->GetWordsNum() == 0)
		return;

	std::vector<int> learnedRecentlyIds;
	double probSub = 0;
	for (int i = 0; i < wordsMgr->GetWordsNum(); ++i)
	{
		if (wordsMgr->isWordLearnedRecently(i))
		{
			learnedRecentlyIds.emplace_back(i);
		}
	}
	double prob = 0;
	if (!learnedRecentlyIds.empty())	{
		prob = std::max(0.5, (wordsToCheck - double(learnedRecentlyIds.size() + 1)) / double(learnedRecentlyIds.size() + 1));  // Выбор между недавно изученными и остальными будет с вероятностью 1 к prob
	}

	// Главный цикл проверки слов
	for (int i = 0; i < wordsToCheck; ++i)
	{
		int id = GetWordIdToCheck(wordsMgr, prob, probSub, learnedRecentlyIds);

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
		int durationForAnswer = static_cast<int>(std::chrono::duration<double, std::milli>(t_end - t_start).count());

		while (true)
		{
			ClearConsoleScreen();
			printf("\n\n===============================\n %s\n===============================\n", wordsMgr->GetWord(id).c_str());
			wordsMgr->printTranslationDecorated(wordsMgr->GetTranslation(id));
			printf("\n\n  Arrow up  - I remember!\n  Arrow down   - I am not sure\n");
			printf("\n  Remain: %d, Time = %d\n", wordsToCheck - i - 1, durationForAnswer);

			c = GetchFiltered();
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх
			{
				ProcessRemember(wordsMgr, id, durationForAnswer);
			}
			else
				if (c == 80) // Стрелка вниз
				{
					ProcessForget(wordsMgr, id);
				}
				else
					continue;
			logger("%s word = %s, key=%d, Quick=%d, Duration=%f\n", GetTimeInText(time(nullptr)), wordsMgr->GetWord(id).c_str(), c, durationForAnswer, durationForAnswer);
			break;
		}
	}
}
//===============================================================================================
//
//===============================================================================================

bool CheckManager::IsQuickAnswer(std::unique_ptr<WordsManager>& wordsMgr, double milliSec, const char* translation)
{
	int index = wordsMgr->getTranslationsNum(translation) - 1;
	const int timesNum = sizeof(quickAnswerTime) / sizeof(quickAnswerTime[0]);
	ClampMinmax(&index, 0, timesNum - 1);

	return milliSec < quickAnswerTime[index];
}
