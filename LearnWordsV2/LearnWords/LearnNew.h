#pragma once

struct LearnWordsApp;
struct WordsData;
struct AdditionalCheck;


// ѕоставщик отвлекающих слов, перемежающих подучиваемые забытые слова. Ќо выбирает дл€ отвлечени€ слова, которые тоже полезно повторить :
// Ёто слова которые пора повтор€ть или скоро будет пора, также это слова, на которые мы недавно ответили правильно, но долго думали

enum class FromWhatSource
{
	NOT_INITIALIZED,
	FROM_LEANRING_QUEUE,     // —лово из очереди подучиваемых слов
	FROM_REMEMBERED_LONG,    // Ёто слово из тех, что провер€ли в этой сессии программы, ответили хорошо, но очень долго
	FROM_MANDATORY,          // Ёто слово из готовых об€зательной проверке (включа€ небольшое опережение)
	FROM_ADDITIONAL          // Ёто слово получено из AdditionalCheck
};

struct DistractWord
{

	DistractWord(int wordIndex, FromWhatSource wordFromWhatSource) : index(wordIndex), fromWhatSource(wordFromWhatSource) {}

	int index = 0;                                                   // »ндекс слова в _wordsOnDisk
	FromWhatSource fromWhatSource = FromWhatSource::NOT_INITIALIZED; // »сточник слова
};

class DistractWordsSupplier  
{
public:
	DistractWordsSupplier(LearnWordsApp* learnWordsApp, time_t freezedTime);
	DistractWord get_word(time_t freezedTime, AdditionalCheck* pAdditionalCheck);
	bool is_first_cycle();

private:
	LearnWordsApp* _learnWordsApp = nullptr;
	std::vector<DistractWord> distractWords; // —лова дл€ отвлечени€. ≈сли их не хватит, то будем динамически добирать из циклического списка повтора
	int index = 0;                     // »ндекс выдаваемого слова в distractWords
	int returnWordsFromAdditional = 0; // —колько слов нужно вернуть из AdditionalCheck. ≈сли здесь 0, то выдаЄм слова из distractWords.
	bool isFirstCycle = true;          // ѕервый ли это круг обхода по массиву distractWords
};


//  ласс дл€ выучивани€ новых слов и подучивани€ забытых 

struct LearnNew
{
	struct WordToLearn
	{
		WordToLearn() {}
		explicit WordToLearn(int index, FromWhatSource fromWhatSource) : _index(index), _fromWhatSource(fromWhatSource) {}

		int  _index = 0;                 // »ндекс изучаемого слова в WordsOnDisk::_words
		int  _localRightAnswersNum = 0;  //  оличество непрерывных правильных ответов
		FromWhatSource _fromWhatSource = FromWhatSource::NOT_INITIALIZED;
	};

	LearnNew(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void learn_new(time_t freezedTime, AdditionalCheck* pAdditionalCheck);
	void learn_forgotten(time_t freezedTime, AdditionalCheck* pAdditionalCheck);
	
	void print_masked_translation(const char* _str, int symbolsToShowNum);
	void put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, bool needRandomInsert);
	bool are_all_words_learned(std::vector<WordToLearn>& queue);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};

