#include <chrono>
#include <windows.h>
#include <iostream>
#include <random>

// -------------------------------------------------------------------

void _cdecl ExitMsg(const char* text, ...)
{
	static char tmpStr[1024];
	va_list args;
	va_start(args, text);
	vsprintf_s(tmpStr, sizeof(tmpStr), text, args);
	va_end(args);

	OutputDebugStringA(tmpStr);
	exit(1);
}

// -------------------------------------------------------------------

void _cdecl PrintConsole(const char* text, ...)
{
	static char tmpStr[1024];
	va_list args;
	va_start(args, text);
	vsprintf_s(tmpStr, sizeof(tmpStr), text, args);
	va_end(args);

	OutputDebugStringA(tmpStr);
}

// -------------------------------------------------------------------

double GetTimeDbl()
{
	static bool isInited = false;
	static std::chrono::high_resolution_clock::time_point startTime;

	if (!isInited) {
		startTime = std::chrono::high_resolution_clock::now();
		isInited = true;
	}

	std::chrono::high_resolution_clock::time_point curTime = std::chrono::high_resolution_clock::now();

	long long diff = std::chrono::duration_cast<std::chrono::microseconds>(curTime - startTime).count();

	const int scale = 64;  // Миллион делится на 64 без остатка
	return double(diff / scale) / (1000000 / scale);
}

// -------------------------------------------------------------------

//std::mt19937 gen;
std::default_random_engine gen;
std::uniform_real_distribution<double> distrib(0, 1);

//--------------------------------------------------------------------------------------------

int RandI(int min, int max)
{
	int val = (int)(distrib(gen) * (max - min + 1)) + min;
	return val <= max ? val: max;
}

//--------------------------------------------------------------------------------------------

float RandF(float min, float max)
{
	return (float)(distrib(gen) * (max - min) + min);
}

