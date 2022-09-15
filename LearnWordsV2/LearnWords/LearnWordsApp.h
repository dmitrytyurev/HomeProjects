#pragma once
#include <vector>
#include <set>
#include <ctime>
#include "WordsData.h"
#include "LearnNew.h"
#include "Check.h"

struct LearnWordsApp
{
	LearnWordsApp();

	void process(int argc, char* argv[]);
	bool is_quick_answer(double milliSec, const char* translation, bool* ifTooLongAnswer = nullptr, double* extraDurationForAnswer = nullptr);
	void print_buttons_hints(const std::string& str, bool needRightKeyHint);
	void save();
	
	// Вызываются функциями данного класса
	int main_menu_choose_mode();
	time_t get_time();
	int get_translations_num(const char* translation);

	// Поля
	WordsData _wordsOnDisk;
	std::string _fullFileName;
	time_t _freezedTime;
	std::vector<int> _forgottenWordsIndices; // Индексы слов, которые были забыты при последней проверке слов

	LearnNew _learnNew;
	Check    _check;
};

