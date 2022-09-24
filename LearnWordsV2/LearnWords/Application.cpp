#include "stdafx.h"
#include "algorithm"
#include "CommonUtility.h"
#include "Application.h"
#include "FileOperate.h"
#include "Windows.h"
#undef min

extern Log logger;

//===============================================================================================

Application::Application(const std::string& wordsFileName)
{
	_wordsManager = std::make_shared<WordsManager>(wordsFileName);
	_learn = std::make_unique<LearnManager>(_wordsManager);
	_check = std::make_unique<CheckManager>(_wordsManager);
}

//===============================================================================================

void Application::process()
{
	while (true)
	{
		clear_console_screen();

		int keyPressed = main_menu_choose_mode();
		switch (keyPressed)
		{
		case 27:  // ESC
			return;
			break;
		case '1':
			_learn->do_learn(false);
			break;
		case '2':
			_check->do_check();
			break;
		case '3':
			if (!_wordsManager->isForgottenListEmpty())
				_learn->do_learn(true);
			break;
		default:
			break;
		}
	}
}

//===============================================================================================

int Application::main_menu_choose_mode()
{
	printf("\n");
	printf("\n");
	printf("1. Learn new words\n");
	printf("2. Repeat Words\n");
	printf("3. Learn forgotten words\n");
	printf("\n\n");

	if (!_wordsManager->isForgottenListEmpty())
	{
		const std::vector<int>& forgottenWords = _wordsManager->getForgottenList();
		for (const auto& index : forgottenWords)
		{
			printf("==============================================\n%s\n   %s\n", _wordsManager->GetWord(index).c_str(), _wordsManager->GetTranslation(index).c_str());
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
