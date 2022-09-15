#pragma once

struct LearnWordsApp;
struct WordsData;
struct AdditionalCheck;


// Поставщик отвлекающих слов, перемежающих подучиваемые забытые слова. Но выбирает для отвлечения слова, которые тоже полезно повторить :
// Это слова которые пора повторять или скоро будет пора, также это слова, на которые мы недавно ответили правильно, но долго думали

enum class FromWhatSource
{
	NOT_INITIALIZED,
	FROM_LEANRING_QUEUE,     // Слово из очереди подучиваемых слов
};

// Класс для выучивания новых слов и подучивания забытых 

struct LearnNew
{
	struct WordToLearn
	{
		WordToLearn() {}
		explicit WordToLearn(int index, FromWhatSource fromWhatSource) : _index(index), _fromWhatSource(fromWhatSource) {}

		int  _index = 0;                 // Индекс изучаемого слова в WordsOnDisk::_words
		int  _localRightAnswersNum = 0;  // Количество непрерывных правильных ответов
		FromWhatSource _fromWhatSource = FromWhatSource::NOT_INITIALIZED;
	};

	LearnNew(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void learn_new();
	
	void print_masked_translation(const char* _str, int symbolsToShowNum);
	void put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, bool needRandomInsert);
	bool are_all_words_learned(std::vector<WordToLearn>& queue);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};

