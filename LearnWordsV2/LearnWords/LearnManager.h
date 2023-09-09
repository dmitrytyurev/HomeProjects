#pragma once
#include <memory>
#include "CheckManager.h"
#include "WordsManager.h"

// Класс для выучивания новых слов и подучивания забытых 

class LearnManager
{
public:
	LearnManager() = default;
	void DoLearn(bool isLearnForgotten, std::unique_ptr<WordsManager>& wordsManager, std::unique_ptr<CheckManager>& checkManager);

private:
	struct WordToLearn
	{
		WordToLearn() {}
		explicit WordToLearn(int index) : _index(index) {}

		int  _index = 0;                 // Индекс изучаемого слова в WordsOnDisk::_words
		int  _localRightAnswersNum = 0;  // Количество непрерывных правильных ответов
	};

	void PrintMaskedTranslation(const char* _str, int symbolsToShowNum);
	void PutToQueue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, bool needRandomInsert);
	bool AreAllWordsLearned(std::vector<WordToLearn>& queue);
};

