#include "pch.h"
#include <stdarg.h>
#include <string>
#include <algorithm>
#include <random>

//--------------------------------------------------------------------------------------------

//std::mt19937 gen;
//std::default_random_engine gen;
//std::uniform_real_distribution<double> distrib(0, 1);

//--------------------------------------------------------------------------------------------

void _cdecl exit_msg(const char *text, ...)
{
	static char tmpStr[1024];
	va_list args;
	va_start(args, text);
	vsprintf_s(tmpStr, sizeof(tmpStr), text, args);
	va_end(args);

	printf("%s", tmpStr);
	exit(1);
}

//--------------------------------------------------------------------------------------------

void log(const std::string& str)
{
	FILE* f = fopen("Error.txt", "at");
	fprintf(f, str.c_str());
	fclose(f);
}

//--------------------------------------------------------------------------------------------

//int rand(int min, int max)
//{
//	int val = (int)(distrib(gen) * (max - min + 1)) + min;
//	return std::min(val, max);
//}
//
////--------------------------------------------------------------------------------------------
//
//float randf(float min, float max)
//{
//	return (float)(distrib(gen) * (max - min) + min);
//}
//
////--------------------------------------------------------------------------------------------
//
//void setRandSeed(unsigned int seed)
//{
//	gen.seed(seed);
//}

