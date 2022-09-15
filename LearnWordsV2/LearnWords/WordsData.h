#pragma once

#include "stdafx.h"
#include <vector>
#include <string>

struct WordsData
{
	struct WordInfo
	{
		std::string word;
		std::string translation;
		int checkOrderN = 0;      // Глобальный номер выполненной проверки. Это число больше у слов, знание которых проверялось позже
		int successCheckDays = 0; // Число дней, когда проходили успешные проверки
		int lastDaySuccCheckTimestamp = 0;  // Таймстемп последней успешной проверки, но обновляется не чаще раза в сутки. Нужен для инкремента successCheckDays
		bool needSkip = false;    // Ставится при быстрой успешной проверки, служит для пропуска одного круга проверки
	};

	std::vector<int> GetUnlearnedTextsId(int textsNumNeeded);
	WordInfo& GetWordInfo(int id);
	std::string GetWord(int id);
	std::string GetTranslation(int id);
	void SetTextAsJustLearned(int id);
	void SetTextAsUnlearned(int id);
	void PutTextToEndOfQueue(int id);

	std::vector<WordInfo> _words;
};
