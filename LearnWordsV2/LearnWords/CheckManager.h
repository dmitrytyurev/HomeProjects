#pragma once
#include <memory>
#include "WordsManager.h"

class CheckManager
{
public:
	CheckManager(std::weak_ptr<WordsManager> pWordsData) : _pWordsData(pWordsData) {}
	void do_check();

private:
	bool is_quick_answer(double milliSec, const char* translation, bool* ifTooLongAnswer = nullptr, double* extraDurationForAnswer = nullptr);
	int get_word_id_to_check(std::vector<int>& lastCheckedIds);
	int get_word_id_to_check_impl(const std::vector<int>& lastCheckedIds);

private:
	std::weak_ptr<WordsManager> _pWordsData;
};
