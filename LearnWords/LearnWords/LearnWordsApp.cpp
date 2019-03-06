#include "stdafx.h"
#include "algorithm"
#include "CommonUtility.h"
#include "LearnWordsApp.h"
#include "FileOperate.h"
#include "Windows.h"
#undef min
struct
{
	int min;
	int max;
} quickAnswerTime[] = {{2100, 3500}, {2600, 4000}, {3100, 4500}, {3600, 5000}, {4100, 5500}}; // Время быстрого и долгого ответа в зависимости от числа переводов данного слова

const int MAX_RIGHT_REPEATS_GLOBAL_N = 81;
const int WORDS_LEARNED_GOOD_THRESHOLD = 22; // Число дней в addDaysMin, по которому выбирается индекс, чтобы считать слова хорошо изученными
const int DOWN_ANSWERS_FALLBACK = 20;             // Номер шага, на который откатывается слово при check_by_time, если забыли слово
const int RIGHT_ANSWERS_FALLBACK = 29;            // Номер шага, на который откатывается слово при check_by_time, если помним неуверенно
const float MIN_DAYS_IF_QUICK_ANSWER = 3;         // Если быстрый ответ, то слово не должно появиться быстрее, чем через это количество дней
const int LAST_HOURS_REPEAT_NUM = 7;  // Если на повтор слов мало, то добавим слова из повторённых за столько последних часов
const int MAX_WORDS_TO_CHECK = 60;  // Если слов на обязательную проверку больше, чем это число, то урезаем
const int MAX_WORDS_TO_CHECK2 = 30; // Если слов на обязательную проверку меньше, чем это число, то добавляем до этого числа
const int PRELIMINARY_CHECK_HOURS = 24;  // Слова, которые надо будет проверять через столько часов добавим в Mandatory проверку сейчас, если слов не хватает

float addDaysMin[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
float addDaysMax[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
float addDaysMinSrc [MAX_RIGHT_REPEATS_GLOBAL_N + 1] = {0, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 1, 1, 1, 2, 2, 3, 3, 4, 1, 5, 1, 5, 1, 7, 1, 2, 9, 1, 3, 11, 1, 3, 14, 1, 3, 8, 17, 1, 3, 9, 20, 1, 3, 9, 23, 1, 3, 11, 27, 1, 3, 14, 35, 1, 3, 10, 18, 42, 1, 3, 10, 20, 50, 1, 3, 10, 25, 60, 1, 3, 10, 20, 35, 70, 1, 3, 10, 20, 40, 80, 1, 3, 10, 20, 45, 90};
float addDaysDiffSrc[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 2, 0, 0, 2, 0, 0, 3, 0, 0, 3, 0, 0, 2, 3, 0, 0, 2, 3, 0, 0, 2, 3, 0, 0, 3, 3, 0, 0, 3, 3, 0, 0, 2, 3, 4, 0, 0, 2, 3, 4, 0, 0, 2, 3, 5, 0, 0, 2, 3, 4, 5, 0, 0, 2, 3, 4, 5, 0, 0, 2, 3, 4, 5};

extern Log logger;

//===============================================================================================
//
//===============================================================================================

LearnWordsApp::LearnWordsApp(): _additionalCheck(this, &_wordsOnDisk), _mandatoryCheck(this, &_wordsOnDisk), _learnNew(this, &_wordsOnDisk), _freezedTime(0) 
{
	for (int i = 0; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
	{
		float t = addDaysMinSrc[i];
		if (t >= 1)  
			t -= 0.2f;
		addDaysMin[i] = t;
		addDaysMax[i] = t + addDaysDiffSrc[i];
	}
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::save()
{
	FileOperate::save_to_file(_fullFileName.c_str(), &_wordsOnDisk);
}



//===============================================================================================
// Вернуть число переводов в данном слове
//===============================================================================================

int LearnWordsApp::get_translations_num(const char* translation)
{
	const char* p = translation;
	int inBracesNestedCount = 0;
	int commasCount = 0;

	while (*p)
	{
		if (*p == '(')
			++inBracesNestedCount;
		else
			if (*p == ')')
				--inBracesNestedCount;
			else
				if (*p == ',' && inBracesNestedCount == 0)
					++commasCount;
		++p;
	}

	return commasCount + 1;
}

//===============================================================================================
//
//===============================================================================================

bool LearnWordsApp::is_quick_answer(double milliSec, const char* translation, bool* ifTooLongAnswer, double* extraDurationForAnswer)
{
	int index = get_translations_num(translation) - 1;
	const int timesNum = sizeof(quickAnswerTime) / sizeof(quickAnswerTime[0]);
	clamp_minmax(&index, 0, timesNum - 1);

	if (ifTooLongAnswer) 
		*ifTooLongAnswer = milliSec > quickAnswerTime[index].max;

	if (extraDurationForAnswer)
		*extraDurationForAnswer = milliSec - (double)quickAnswerTime[index].min;

	return milliSec < quickAnswerTime[index].min;
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::clear_forgotten()
{
	_forgottenWordsIndices.clear();
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::add_forgotten(int forgottenWordIndex)
{
	if (!is_in_forgotten(forgottenWordIndex))
		_forgottenWordsIndices.push_back(forgottenWordIndex);
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::get_forgotten(std::vector<int>& forgottenWordsIndices)
{
	forgottenWordsIndices = _forgottenWordsIndices;
}

//===============================================================================================
//
//===============================================================================================

bool LearnWordsApp::is_in_forgotten(int wordIndex)
{
	auto result = std::find(_forgottenWordsIndices.begin(), _forgottenWordsIndices.end(), wordIndex);
	return result != _forgottenWordsIndices.end();
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::erase_from_remembered_long(int wordIndex)
{
	WordRememberedLong w(wordIndex, 0);

	auto result = std::find(_wordRememberedLong.begin(), _wordRememberedLong.end(), w);
	if (result != _wordRememberedLong.end())
		_wordRememberedLong.erase(result);
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::update_time_remembered_long(int wordIndex, double durationOfRemember)
{
	WordRememberedLong w(wordIndex, durationOfRemember);

	auto result = std::find(_wordRememberedLong.begin(), _wordRememberedLong.end(), w);
	if (result != _wordRememberedLong.end())
	{
		_wordRememberedLong.erase(result);
		_wordRememberedLong.insert(w);
	}
}

//===============================================================================================
//
//===============================================================================================

bool LearnWordsApp::isWordJustLearnedOrForgotten(const WordsData::WordInfo& w, time_t curTime) const
{
	return (w.rightAnswersNum == 1) &&   // Это условие не даёт попасть в выборку словам высоких уровней, до повторения которых осталось несколько часов
		((w.dateOfRepeat >= curTime) && (w.dateOfRepeat <= curTime + addDaysMax[1] * SECONDS_IN_DAY));  // А это условие не даёт попасть выборку только что изученным словам на следующий день
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::reset_all_words_to_repeated(int rightAnswersToSet, float minDaysRepeat, float maxDaysRepeat, time_t currentTime)
{
	for (auto& w : _wordsOnDisk._words)
	{
		if (w.rightAnswersNum)
		{
			w.rightAnswersNum = rightAnswersToSet;
			float randDays = rand_float(minDaysRepeat, maxDaysRepeat);
			fill_dates(randDays, w, currentTime);
		}
	}
}


//===============================================================================================
// 
//===============================================================================================

int LearnWordsApp::main_menu_choose_mode(time_t freezedTime)
{
//	_additionalCheck.log_random_test_words(_freezedTime);

	int wordsTimeToRepeatNum = 0;
	int wordsByLevel[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
	recalc_stats(_freezedTime, &wordsTimeToRepeatNum, wordsByLevel);

	int wordsLearnGoodIndex = 0;   // Найти индекс, больше которого имеют хорошо изученные слова
	for (int i = 1; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
		if (addDaysMin[i] >= WORDS_LEARNED_GOOD_THRESHOLD)
		{
			wordsLearnGoodIndex = i;
			break;
		}
	int wordsLearnedTotal = 0;
	int wordsLearnedGood = 0;
	for (int i = 1; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
	{
		wordsLearnedTotal += wordsByLevel[i];
		if (i >= wordsLearnGoodIndex)
			wordsLearnedGood += wordsByLevel[i];
	}
	static int prevWordsLearnedGood;
	int deltaWordsLearnedGood = 0;
	if (prevWordsLearnedGood > 0)
		deltaWordsLearnedGood = wordsLearnedGood - prevWordsLearnedGood;
	prevWordsLearnedGood = wordsLearnedGood;

	printf("\n");
	printf("\n");
	printf("1. Выучить новые слова\n");
	printf("2. Ежедневный повтор  [%d]\n", wordsTimeToRepeatNum);
	printf("\n\n");

	printf("Выучено слов: %d, из них хорошо: %d (%d)\n", wordsLearnedTotal, wordsLearnedGood, deltaWordsLearnedGood);
	logger("Learned and good: %d, %d, time = %s", wordsLearnedTotal, wordsLearnedGood, get_time_in_text(time(nullptr)));

//	printf("  Рандомный повтор: Основн=%d (из них skip=%d (%d)), Быстрая=%d ", mainQueueLen, mainQueueSkipLoopCount, deltaSkipLoopCount, fastQueueLen);

	if (!_forgottenWordsIndices.empty())
	{
		for (const auto& index : _forgottenWordsIndices)
		{
			WordsData::WordInfo& w = _wordsOnDisk._words[index];
			printf("==============================================\n%s\n   %s\n", w.word.c_str(), w.translation.c_str());
		}
	}
	else
	{
		auto print_upcoming_words_info = [this, wordsLearnGoodIndex, freezedTime](int stagesBehind)
		{
			for (const auto& w : _wordsOnDisk._words)
			{
				if (w.rightAnswersNum == wordsLearnGoodIndex - stagesBehind)
					printf("%d ", int((w.dateOfRepeat - freezedTime) / 3600 / 24.f));
			}
		};

		printf("\nСколько дней до проверки кандидатов на хорошо выученные слова");
		printf("\n-7 этапа: ");
		print_upcoming_words_info(7);
		printf("\n-6 этапа: ");
		print_upcoming_words_info(6);
		printf("\n-5 этапа: ");
		print_upcoming_words_info(5);
		printf("\n-4 этапа: ");
		print_upcoming_words_info(4);
		printf("\n-3 этапа: ");
		print_upcoming_words_info(3);
		printf("\n-2 этапа: ");
		print_upcoming_words_info(2);
		printf("\n-1 этап: ");
		print_upcoming_words_info(1);
	}
	
	while (true)
	{
		char c = getch_filtered();
		if (c == 27 || c == '1' || c == '2')  // 27 - Esc
			return c;
		//		printf("%d\n", c);
	}

	return 0;
}


//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::recalc_stats(time_t curTime, int* wordsTimeToRepeatNum, int wordsByLevel[])
{
	*wordsTimeToRepeatNum = 0;

	for (int i = 0; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
		wordsByLevel[i] = 0;

	for (const auto& w : _wordsOnDisk._words)
	{
		++(wordsByLevel[w.rightAnswersNum]);

		if (w.dateOfRepeat != 0)
		{
			if (w.rightAnswersNum == 0)
				exit_msg("Semantic error\n");

			if (w.dateOfRepeat < curTime)
				++(*wordsTimeToRepeatNum);
		}
	}

	//	printf("\n%d  - Words that is time to repeat\n\n", wordsTimeToRepeatNum);
}


//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::fill_dates(float randDays, WordsData::WordInfo &w, time_t currentTime)
{
	int secondsPlusCurTime = int(randDays * SECONDS_IN_DAY);
	w.dateOfRepeat = (int)currentTime + secondsPlusCurTime;
	w.cantRandomTestedAfter = (int)currentTime + secondsPlusCurTime / 2;
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::fill_dates_and_save(WordsData::WordInfo& w, time_t currentTime, bool needAdvance_RightAnswersNum, bool isQuickAnswer)
{
	if (needAdvance_RightAnswersNum)
	{
		++w.rightAnswersNum;
		clamp_max(&w.rightAnswersNum, MAX_RIGHT_REPEATS_GLOBAL_N);
	}

	float min = addDaysMin[w.rightAnswersNum];
	float max = addDaysMax[w.rightAnswersNum];

	if (isQuickAnswer)
		min = (min + max) * 0.5f;
	else
		max = (min + max) * 0.5f;

	float randDays = rand_float(min, max);

	if (isQuickAnswer && randDays < MIN_DAYS_IF_QUICK_ANSWER)
	{
		while (w.rightAnswersNum < MAX_RIGHT_REPEATS_GLOBAL_N && addDaysMax[w.rightAnswersNum] < MIN_DAYS_IF_QUICK_ANSWER)
			++w.rightAnswersNum;
		randDays = MIN_DAYS_IF_QUICK_ANSWER;
	}

	fill_dates(randDays, w, currentTime);
	save();
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::print_buttons_hints(const std::string& str, bool needRightKeyHint)
{
	CONSOLE_SCREEN_BUFFER_INFO   csbi;
	csbi.wAttributes = 0;
	bool isColorReadSucsess = false;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole != INVALID_HANDLE_VALUE)
		isColorReadSucsess = GetConsoleScreenBufferInfo(hConsole, &csbi) == TRUE;

	const char* p = str.c_str();
	int bracesNestCount = 0;

	printf("\n");
	if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
		SetConsoleTextAttribute(hConsole, 15);
	if (str.find('[') == std::string::npos)
		printf(" ");

	while (*p)
	{
		if (*p == '(' || *p == '[')
		{
			++bracesNestCount;
			if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
				SetConsoleTextAttribute(hConsole, 8);
		}

		bool isCommaSeparatingMeanings = (*p == ',' &&  bracesNestCount == 0);
		if (!isCommaSeparatingMeanings)
			printf("%c", *p);

		if (*p == ')' || *p == ']')
		{
			--bracesNestCount;
			if (bracesNestCount == 0)
				if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
					SetConsoleTextAttribute(hConsole, 15);
		}

		if (*p == ']' || (*p == ',' && bracesNestCount == 0))
		{ 
			printf("\n");
			if (*p == ']')
				printf("\n");
		}

		++p;
	}
	printf("\n");

	if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
		SetConsoleTextAttribute(hConsole, csbi.wAttributes);

	printf("\n\n  Стрелка вверх  - помню хорошо!\n  Стрелка вниз   - забыл/перепутал хотя бы одно значение\n");
	if (needRightKeyHint)
		printf("  Стрелка вправо - вспомнил все значения, но с трудом\n");
}


//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::set_word_as_just_learned(WordsData::WordInfo& w)
{
	w.clear_all();
	w.rightAnswersNum = 1;
}

//===============================================================================================
// 
//===============================================================================================

time_t LearnWordsApp::get_time()
{
//return 1505167116;  // Отладочный режим со стабильным временем и рандомом. При тесте отвечать на вопросы надо быстро для одинакового результата
	return std::time(nullptr);
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::process(int argc, char* argv[])
{
	std::srand((unsigned)get_time());
	srand((int)get_time());

	if (argc != 3)
	{
		//puts("Ussage:");
		//puts("LearnWords.exe [path to base file]\n");
		//return 0;
		_fullFileName = "C:\\Dimka\\yadisk\\LearnWords\\dima_to_learn.txt";
		_fullRimPath = "C:\\Dimka\\MyLims\\40\\CNN\\CBSBenghaziProblem\\";
	}
	else
	{
		_fullFileName = argv[1];
		_fullRimPath = argv[2];
	}
	FileOperate::load_from_file(_fullFileName.c_str(), &_wordsOnDisk);
	//	wordsOnDisk.export_for_google_doc();

	//FileOperate::save_to_file(_fullFileName.c_str(), &_wordsOnDisk);
	//return;

	while (true)
	{
		clear_console_screen();

		_freezedTime = get_time();   // Текущее время обновляется один раз перед показом главного меню, чтобы число слов для повтора в меню 
								   // и последующем запуске режима повтора (в нём используется запомненный здесь curTime) было одинаковым .
		int keyPressed = main_menu_choose_mode(_freezedTime);
		switch (keyPressed)
		{
		case 27:  // ESC
			printf("%ld\n", int(_freezedTime));
			return;
			break;
		case '1':
			_learnNew.learn_new(_freezedTime, &_additionalCheck);
			break;
		case '2':
			_mandatoryCheck.mandatory_check(_freezedTime, &_additionalCheck, _fullFileName);
			if (!_forgottenWordsIndices.empty())
				do 
				{
					_learnNew.learn_forgotten(_freezedTime, &_additionalCheck);
				} while (!_forgottenWordsIndices.empty());
			break;
		default:
			break;
		}
	}
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::set_as_forgotten(WordsData::WordInfo& w)
{
	w.rightAnswersNum = std::min(w.rightAnswersNum, DOWN_ANSWERS_FALLBACK);
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::set_as_barely_known(WordsData::WordInfo& w)
{
	w.rightAnswersNum = std::min(w.rightAnswersNum, RIGHT_ANSWERS_FALLBACK);
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::collect_words_to_mandatory_check(std::vector<WordToCheck>& wordsToRepeat, time_t freezedTime)
{
	auto ifTimeToRepeat = [](WordsData::WordInfo& w, time_t freezedTime) { return w.dateOfRepeat != 0 && w.dateOfRepeat < freezedTime; };

	for (int i = 0; i < (int)_wordsOnDisk._words.size(); ++i)
	{
		WordsData::WordInfo& w = _wordsOnDisk._words[i];
		if (ifTimeToRepeat(w, freezedTime))
			wordsToRepeat.emplace_back(WordToCheck(i));
	}

	// Если этих слов больше, чем нужно, то урезать число слов до необходимого

	if (wordsToRepeat.size() > MAX_WORDS_TO_CHECK)
	{
		for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
		{
			WordsData::WordInfo& w = _wordsOnDisk._words[wordsToRepeat[i]._index];
			int plannedRepeatInterval = (w.dateOfRepeat - w.cantRandomTestedAfter) * 2;;  // Запланированный интервал повтора
			int lateRepeatInterval = int(freezedTime) - w.dateOfRepeat;  // На сколько просрочен повтор от запланированного времени
			wordsToRepeat[i]._sortCoeff = lateRepeatInterval / (plannedRepeatInterval * 0.2f + 1);  // Чем больше коэффициент, тем раньше надо повторять слова
		}
		std::sort(wordsToRepeat.begin(), wordsToRepeat.end(), [](const WordToCheck& l, const WordToCheck& r) { return l._sortCoeff > r._sortCoeff; });
		wordsToRepeat.resize(MAX_WORDS_TO_CHECK);
	}
	else
		if (wordsToRepeat.size() < MAX_WORDS_TO_CHECK2)  // Если слов наоборот меньше, то добавить слова, которые надо будет проверять через PRELIMINARY_CHECK_HOURS часов
		{
			auto ifRepeatedRecently = [](WordsData::WordInfo& w, time_t freezedTime) { return freezedTime - w.calcPrevRepeatTime() < 3600 * LAST_HOURS_REPEAT_NUM; };

			for (int i = 0; i < (int)_wordsOnDisk._words.size(); ++i)
			{
				WordsData::WordInfo& w = _wordsOnDisk._words[i];

				if (!ifTimeToRepeat(w, freezedTime) && !ifRepeatedRecently(w, freezedTime))
				{
					if (w.dateOfRepeat != 0 && w.dateOfRepeat < freezedTime + 3600 * PRELIMINARY_CHECK_HOURS)
					{
						logger("Add from future: %s, time to repeat: %d\n", w.word.c_str(), w.dateOfRepeat - freezedTime);
						wordsToRepeat.emplace_back(WordToCheck(i));
						if (wordsToRepeat.size() == MAX_WORDS_TO_CHECK2)
							break;
					}
				}
			}
		}
}
