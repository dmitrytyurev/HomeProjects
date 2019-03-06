#pragma once
#include <vector>
#include "WordsData.h"

struct LearnWordsApp;
struct WordsData;

struct AdditionalCheck
{
	AdditionalCheck(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void additional_check(time_t freezedTime, const std::string& fullFileName);

	bool can_random_tested(WordsData::WordInfo& w, time_t freezedTime) const;
	int calc_max_randomTestIncID(bool isFromFastQueue);
	void fill_indices_of_random_repeat_words(std::vector<int> &indicesOfWords, bool isFromFastQueue, time_t freezedTime);
	void calc_words_for_random_repeat(int* mainQueueLen, int* mainQueueSkipLoopCount, int* fastQueueLen, time_t freezedTime);
	void log_random_test_words(time_t freezedTime);
	int get_word_to_repeat_inner(time_t freezedTime);
	int get_word_to_repeat(time_t freezedTime);
	void put_word_to_end_of_random_repeat_queue_common(WordsData::WordInfo& w);
	void put_word_to_end_of_random_repeat_queue_fast(WordsData::WordInfo& w, time_t currentTime);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};