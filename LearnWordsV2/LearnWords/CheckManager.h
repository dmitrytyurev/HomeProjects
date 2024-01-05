#pragma once
#include <memory>
#include "WordsManager.h"

class CheckManager
{
public:
	CheckManager() = default;
	void DoCheck(std::unique_ptr<WordsManager>& wordsManager);
	int GetWordIdToCheck(std::unique_ptr<WordsManager>& wordsManager, double prob, double& probSub, std::vector<int>& learnedRecentlyIds);
	void ProcessRemember(std::unique_ptr<WordsManager>& wordsMgr, int id, int timeToAnswer);
	void ProcessForget(std::unique_ptr<WordsManager>& wordsMgr, int id);

private:
	bool IsQuickAnswer(std::unique_ptr<WordsManager>& wordsManager, double milliSec, const char* translation);
};
