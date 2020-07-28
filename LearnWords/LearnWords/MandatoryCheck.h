#pragma once

struct MandatoryCheck
{
	MandatoryCheck(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void mandatory_check(time_t freezedTime, AdditionalCheck* pAdditionalCheck, const std::string& fullFileName);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};
