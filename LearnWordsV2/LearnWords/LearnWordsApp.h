#pragma once
#include <vector>
#include <set>
#include <ctime>
#include "WordsData.h"
#include "LearnNew.h"

struct WordToCheck
{
	WordToCheck() : _index(0), _sortCoeff(0) {}
	WordToCheck(int index) : _index(index), _sortCoeff(0) {}

	int   _index;       // Индекс повторяемого слова в WordsOnDisk::_words
	float _sortCoeff;   // Сортировочный коэффициент, если пора повторять больше слов, чем мы хотим сейчас повторять
};

struct WordRememberedLong  // Слово, которое человек вспомнил правильно, но вспоминал долго
{
	WordRememberedLong(int wordIndex, double wordDuration) : index(wordIndex), durationOfRemember(wordDuration) {}
	bool operator<(const WordRememberedLong& r) const {
		return index < r.index;
	}
	bool operator==(const WordRememberedLong& r) const {
		return index == r.index;
	}

	int index = 0;                 // Индекс слова в _wordsOnDisk
	double durationOfRemember = 0; // Длительность вспоминания слова
};

struct LearnWordsApp
{
	LearnWordsApp();

	void process(int argc, char* argv[]);
	bool is_quick_answer(double milliSec, const char* translation, bool* ifTooLongAnswer = nullptr, double* extraDurationForAnswer = nullptr);
	void print_buttons_hints(const std::string& str, bool needRightKeyHint);
	void save();
	
	// Вызываются функциями данного класса
	int main_menu_choose_mode(time_t freezedTime);
	time_t get_time();
	int get_translations_num(const char* translation);

	// Поля
	WordsData _wordsOnDisk;
	std::string _fullFileName;
	std::string _fullRimPath;
	time_t _freezedTime;
	std::vector<int> _forgottenWordsIndices; // Индексы слов, которые были забыты при последней проверке слов
	std::set<WordRememberedLong> _wordRememberedLong; // Слова которые при проверки были вспомнены, но вспоминалсь долго

	LearnNew        _learnNew;
};

