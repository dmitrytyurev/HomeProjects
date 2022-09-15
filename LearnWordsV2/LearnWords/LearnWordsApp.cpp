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

LearnWordsApp::LearnWordsApp(): _learnNew(this, &_wordsOnDisk), _check(this, &_wordsOnDisk), _freezedTime(0)
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

int LearnWordsApp::main_menu_choose_mode()
{
	printf("\n");
	printf("\n");
	printf("1. Learn new words\n");
	printf("2. Words repeat\n");
	printf("3. Learn forgotten\n");
	printf("\n\n");

	if (!_forgottenWordsIndices.empty())
	{
		for (const auto& index : _forgottenWordsIndices)
		{
			WordsData::WordInfo& w = _wordsOnDisk._words[index];
			printf("==============================================\n%s\n   %s\n", w.word.c_str(), w.translation.c_str());
		}
	}
	
	while (true)
	{
		char c = getch_filtered();
		if (c == 27 || c == '1' || c == '2' || c == '3')  // 27 - Esc
			return c;
		//		printf("%d\n", c);
	}

	return 0;
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

	printf("\n\n  Arrow up  - I remember!\n  Arrow down   - I am not sure\n");
	if (needRightKeyHint)
		printf("  Arrow right - I remember but it takes time\n");
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
	if (argc != 2)
	{
		//puts("Ussage:");
		//puts("LearnWords.exe [path to base file]\n");
		//return 0;
		_fullFileName = "D:\\HomeProjects\\LearnWordsV2\\Words.txt";
	}
	else
	{
		_fullFileName = argv[1];
	}
	FileOperate::load_from_file(_fullFileName.c_str(), &_wordsOnDisk);

	while (true)
	{
		clear_console_screen();

		int keyPressed = main_menu_choose_mode();
		switch (keyPressed)
		{
		case 27:  // ESC
			//printf("%ld\n", int(_freezedTime));
			return;
			break;
		case '1':
			_learnNew.learn_new();
			break;
		case '2':
			_check.check();
			break;
		case '3':
			//_learnNew.learn_new(_freezedTime);
			break;
		default:
			break;
		}
	}
}

