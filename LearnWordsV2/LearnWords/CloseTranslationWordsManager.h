#pragma once
#include <vector>
#include <string>

struct WordsData;
struct LearnWordsApp;

//===============================================================================================
// 
//===============================================================================================

class CloseTranslationWordsManager
{
public:
	explicit CloseTranslationWordsManager(LearnWordsApp* learnWordsApp, WordsData* pWordsData, int wordIndex) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData), _srcWordIndex(wordIndex) {}
	void print_close_words_by_translation();
	void process_user_input(char c);

private:
	struct CloseWordFound
	{
		std::string rusWordSrc;  // Для какого русского слова было найдено английское слово с похожим переводом
		std::string engWord;     // Какой английское слово было найдено
		std::string translation; // Полный перевод этого английского слова
	};

	void add_exclusion(int n);
	void add_close_eng_word_to_translation(int n);
	void get_separate_words_from_translation(const char* __str, std::vector<std::string>& outWords);
	bool is_word_in_filter_already(const std::string& word1, const std::string& word2);
	bool is_word_appended_to_translation_already(const std::string& engWord) const;
	void collect_close_words_to(std::vector<CloseWordFound>& closeWordsFound, const std::string& rusWord, int srcWordIndex, const std::string& engWord);

private:
	LearnWordsApp* _learnWordsApp;
	WordsData* _pWordsData;

	int _srcWordIndex;
	std::vector<CloseWordFound> _closeWordsFound;
};
