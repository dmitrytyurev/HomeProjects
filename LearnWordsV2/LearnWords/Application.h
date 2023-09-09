#pragma once
#include <vector>
#include <set>
#include <ctime>
#include <memory>

#include "WordsManager.h"
#include "LearnManager.h"
#include "CheckManager.h"

class Application
{
public:
	Application(const std::string& wordsFileName);
	void Process();
	
private:
	int MainMenuChooseMode();

private:
	std::unique_ptr<WordsManager> _wordsManager;  // База слов с их состояниями и типичные операции над ними
	std::unique_ptr<LearnManager> _learn;         // Режим выучивания и подучивания забытых слов
	std::unique_ptr<CheckManager> _check;         // Режим проверки - помнит ли пользователь слова
};

