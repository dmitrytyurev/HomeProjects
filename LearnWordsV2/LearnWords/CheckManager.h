#pragma once
#include <memory>
#include "WordsManager.h"

class CheckManager
{
public:
	CheckManager(std::weak_ptr<WordsManager> pWordsData) : _pWordsData(pWordsData) {}
	void DoCheck();

private:
	bool IsQuickAnswer(double milliSec, const char* translation, bool* ifTooLongAnswer = nullptr, double* extraDurationForAnswer = nullptr);
	int GetWordIdToCheck(std::vector<int>& lastCheckedIds);
	int GetWordIdToCheckImpl(const std::vector<int>& lastCheckedIds);

private:
	std::weak_ptr<WordsManager> _pWordsData;
};
