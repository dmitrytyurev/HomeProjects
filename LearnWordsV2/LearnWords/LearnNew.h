#pragma once

struct LearnWordsApp;
struct WordsData;
struct AdditionalCheck;

// Класс для выучивания новых слов и подучивания забытых 

struct LearnNew
{
	struct WordToLearn
	{
		WordToLearn() {}
		explicit WordToLearn(int index) : _index(index) {}

		int  _index = 0;                 // Индекс изучаемого слова в WordsOnDisk::_words
		int  _localRightAnswersNum = 0;  // Количество непрерывных правильных ответов
	};

	LearnNew(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void do_learn(bool isLearnForgotten);
	
	void print_masked_translation(const char* _str, int symbolsToShowNum);
	void put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, bool needRandomInsert);
	bool are_all_words_learned(std::vector<WordToLearn>& queue);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};

