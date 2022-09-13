#include "stdafx.h"

#include <clocale>

#include "CommonUtility.h"
#include "LearnWordsApp.h"

//===============================================================================================
// 
//===============================================================================================

const char* logFileName = "log.log";
Log logger(logFileName);

//===============================================================================================
// 
//===============================================================================================

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");

	LearnWordsApp app;
	app.process(argc, argv);

    return 0; 
}


