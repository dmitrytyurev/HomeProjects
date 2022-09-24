#include "StdAfx.h"
#include "Windows.h"
#include "HightResolutionTimer.h"

//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

HightResolutionTimer::HightResolutionTimer(): _startValue(0), _coeff(0)
{
}

//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

void HightResolutionTimer::Start(int ticsPerSecond)
{
	LARGE_INTEGER res;

	QueryPerformanceCounter(&res);
	_startValue = res.QuadPart;

	QueryPerformanceFrequency(&res);
	_coeff = double(ticsPerSecond) / double(res.QuadPart);
}

//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

HightResolutionTimer::HightResolutionTimer(int ticsPerSecond)
{
	Start(ticsPerSecond);
}

//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

__int64 HightResolutionTimer::Get() const
{
	LARGE_INTEGER res;
	QueryPerformanceCounter(&res);
	double curTime = double(res.QuadPart - _startValue) * _coeff;

	return __int64(curTime);
}


//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

double HightResolutionTimer::GetDbl() const
{
	LARGE_INTEGER res;
	QueryPerformanceCounter(&res);

	return  double(res.QuadPart - _startValue) * _coeff;
}


