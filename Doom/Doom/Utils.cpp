#include <windows.h>
#include <iostream>

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
