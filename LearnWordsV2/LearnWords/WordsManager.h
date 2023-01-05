#pragma once

#include "stdafx.h"
#include <vector>
#include <string>

class WordsManager
{
public:
	struct WordInfo
	{
		std::string word;
		std::string translation;
		int checkOrderN = 0;      // Глобальный номер выполненной проверки. Это число больше у слов, знание которых проверялось позже
		int successCheckDays = 0; // Число дней, когда проходили успешные проверки. Если == 0, значит слово ещё не изучали
		int lastDaySuccCheckTimestamp = 0;  // Таймстемп последней успешной проверки, но обновляется не чаще раза в сутки. Нужен для инкремента successCheckDays
	};

	WordsManager(const std::string& wordsFileName);
	void save();
	void printTranslationDecorated(const std::string& str);
	// Доступ к данным о словах
	std::string GetWord(int id);
	std::string GetTranslation(int id);
	int GetWordsNum();
	WordInfo& GetWordInfo(int id);
	std::vector<int> GetUnlearnedTextsId(int textsNumNeeded);
	int getTranslationsNum(const char* translation);
	bool isWordLearnedRecently(int id);
	// Установка свойств слова
	void SetWordAsJustLearned(int id);
	void SetWordAsUnlearned(int id);
	void PutWordToEndOfQueue(int id, bool wasQuickAnswer);
	// Работа со списком забытых при проверке слов
	void clearForgottenList();
	bool isForgottenListEmpty();
	void addElementToForgottenList(int id);
	const std::vector<int>& getForgottenList();

private:
	void load(const std::string& wordsFileName);

private:
	std::string _fullFileName;
	std::vector<int> _forgottenWordsIndices; // Индексы слов, которые были забыты при последней проверке слов
	std::vector<WordInfo> _words;            // База всех слов
};
