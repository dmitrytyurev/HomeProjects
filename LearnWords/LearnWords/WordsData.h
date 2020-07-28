#pragma once

#include "stdafx.h"
#include <vector>
#include <string>

struct WordsData
{
	struct WordInfo
	{
		WordInfo() { clear_all(); }
		void clear_all()
		{
			rightAnswersNum = 0;
			dateOfRepeat = 0;
			cantRandomTestedAfter = 0;
			randomTestIncID = 0;
			cantRandomTestedBefore = 0;
			isInFastRandomQueue = false;
			isNeedSkipOneRandomLoop = false;
		}
		int calcPrevRepeatTime()  // !!! Внимание тут используется факт, что при расчёте cantRandomTestedAfter берётся половина срока до повтора
		{
			return cantRandomTestedAfter * 2 - dateOfRepeat;
		}

		std::string word;
		std::string translation;
		int rightAnswersNum;       // По-дефолту 0 - если слово ещё не учили
								   // Сколько раз слово было успешно проверено. После первичного изучения rightAnswersNum = 1. 
		int dateOfRepeat;          // Дата, когда нужно повторить слово (время в секундах с 1970 года). По-дефолту = 0. Имеет смысл, если rightAnswersNum >= 1

		int cantRandomTestedAfter; // Если !=0, то после этого времени нельзя использовать слово в рандомном тесте, ведь скоро уже плановая проверка слова. Иначе будет не чистый тест.
		int randomTestIncID;       // У только что рандомно повторённого слова ставится на 1 больше, чем у всех имеющихся. 
		bool isInFastRandomQueue;  // Находится ли слово в быстрой очереди рандомного повтора
		bool isNeedSkipOneRandomLoop; // Нужно ли пропустить один круг рандомного повтора слова
		int  cantRandomTestedBefore;  // Если !=0, то до этого времени нельзя использовать слово в рандомном тесте, поскольку мы недавно его повторяли и нужно выждать немного перед ещё одним повтором
	};

	struct CompareExcludePair
	{
		std::string word1;
		std::string word2;
	};

	struct ListeningTextToKeep
	{
		std::string rimFolder; // Путь к лим-каталогу, из которого фраза
		int volumeN;           // Номер главы (соответствует названию папки в лим-каталоге
		int phraseN;           // Номер фразы в главе
	};

	std::vector<WordInfo> _words;
	std::vector<CompareExcludePair> _compareExcludePairs;
	std::vector<ListeningTextToKeep> _listeningTextsToKeep;
};