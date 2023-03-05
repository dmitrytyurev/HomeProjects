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
	int GetWordIdToCheck(int prob, double& probSub);

private:
	std::weak_ptr<WordsManager> _pWordsData;
};
