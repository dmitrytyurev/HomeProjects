#include "stdafx.h"

#include <clocale>

#include "CommonUtility.h"
#include "Application.h"

//===============================================================================================

const char* logFileName = "log.log";
Log logger(logFileName);

//===============================================================================================

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");

	std::string wordsFileName = "../Words.txt";
	if (argc != 2)
	{
		//puts("Ussage:");
		//puts("LearnWords.exe [path to base file]\n");
		//return 0;
	}
	else
	{
		wordsFileName = argv[1];
	}

	Application app(wordsFileName);
	app.Process();

    return 0; 
}


