#pragma once

struct Check
{
	Check(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void check();

	int get_word_id_to_check(std::vector<int>& lastCheckedIds);
	int get_word_id_to_check_impl(const std::vector<int>& lastCheckedIds);


	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};
