#pragma once

//-----------------------------------------------------------------------------
// Класс HightResolutionTimer реализует абстракцию таймера.
// Возвращает текущее время. Частота увеличения таймера задаётся в конструкторе.
//-----------------------------------------------------------------------------

class HightResolutionTimer
{
public:
	HightResolutionTimer();
	HightResolutionTimer(int ticsPerSecond);        // Задаётся количество типов в секунду
	void Start(int ticsPerSecond);
	__int64 Get() const;                            // Получить текущее значение таймера в целочисленном виде
	double  GetDbl() const;                        // Получить текущее значение таймера в виде double
	//static __int64 get_cpu_clock() { __asm rdtsc }  // Получить процессорный счётчик тактов 

private:
	__int64 _startValue;
	double  _coeff;
};
