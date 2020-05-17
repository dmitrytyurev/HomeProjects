#include "pch.h"
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <algorithm>
#include "bmp.h"

//--------------------------------------------------------------------------------------------

const int SceneSize = 200;
const float Scale = 1.1f;

//--------------------------------------------------------------------------------------------

struct Cell
{
	float smokeDens = 0;
	float smokeDensAdd = 0;
	float speedX = 0;
	float speedY = 0;
};

//--------------------------------------------------------------------------------------------

Cell scene[SceneSize][SceneSize];

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

void saveSceneToBmp(const std::string& fileName)
{
	uint8_t* bmpData = new uint8_t[SceneSize * SceneSize * 3];
	for (int y=0; y < SceneSize; ++y)
	{
		for (int x = 0; x < SceneSize; ++x)
		{
			uint8_t bright = std::min(int(scene[x][y].smokeDens * 255), 255);
			int pixelOffs = (y * SceneSize + x) * 3;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
		}
	}
	
	save_bmp24(fileName.c_str(), SceneSize, SceneSize, (const char *)bmpData);
	delete[] bmpData;
}

//--------------------------------------------------------------------------------------------

void update()
{
	std::vector<float> coeffs; // Коэффициенты зацепления заффекченных клатов для нормализации

	for (int y = 0; y < SceneSize; ++y) {
		for (int x = 0; x < SceneSize; ++x) {
			// Расчитаем новые координаты углов текущей клетки после сдвига и расширения
			float x1 = (float)x;
			float y1 = (float)y;
			float x2 = x1 + 1.f;
			float y2 = y1 + 1.f;
			float xCenter = x1 + 0.5f;
			float yCenter = y1 + 0.5f;
			x1 = (x1 - xCenter) * Scale + xCenter + scene[x][y].speedX;
			y1 = (y1 - yCenter) * Scale + yCenter + scene[x][y].speedY;
			x2 = (x2 - xCenter) * Scale + xCenter + scene[x][y].speedX;
			y2 = (y2 - yCenter) * Scale + yCenter + scene[x][y].speedY;

			// Расчитаем какие клетки (и с каким весом) цепляет сдвинутая и расширенная текущая клетка
			int x1i = ((int)(x1 + 1000)) - 1000;
			int y1i = ((int)(y1 + 1000)) - 1000;
			int x2i = ((int)(x2 + 1000)) - 1000;
			int y2i = ((int)(y2 + 1000)) - 1000;

			coeffs.clear();
			float coeffsSum = 0;
			for (int yi = y1i; yi <= y2i; ++yi) {
				for (int xi = x1i; xi <= x2i; ++xi) {
					float outPartL = 0;   // Внешняя доля зацепленной клетки по-горизонтали слева
					if (xi == x1i) {
						outPartL = x1 - (float)x1i;
					}
					float outPartR = 0;
					if (xi == x2i) {     // Внешняя доля зацепленной клетки по-горизонтали справа
						outPartR = 1 - (x2 - (float)x2i);
					}
					float partHor = 1 - (outPartL + outPartR);  // Внутренняя доля зацепленной клетки по-горизонтали

					float outPartU = 0;   // Внешняя доля зацепленной клетки по-горизонтали слева
					if (yi == y1i) {
						outPartU = y1 - (float)y1i;
					}
					float outPartD = 0;
					if (yi == y2i) {     // Внешняя доля зацепленной клетки по-горизонтали справа
						outPartD = 1 - (y2 - (float)y2i);
					}
					float partVert = 1 - (outPartU + outPartD);  // Внутренняя доля зацепленной клетки по-горизонтали

					float partAll = partHor * partVert;  // Полная доля зацепленной клетки по-вертикали и по-горизонтали
					coeffsSum += partAll;
					coeffs.push_back(partAll); // Сохраняем для последующей нормализации суммы коэффициентов на 1
				}
			}

			// Нормируем все собранные коэффициенты на 1
			for (float& elem : coeffs) {
				elem /= coeffsSum;
			}

			// Вычитаем плотность дыма текущей клетки из своей же клетки (аддитивная карта, будет пременена в конце кадра)
			scene[x][y].smokeDensAdd -= scene[x][y].smokeDens;

			// Прибавляем плотность дыма текущей клетки ко всем заффекченным клеткам с нормированными коэффициентами перекрытия по клеткам периметра (аддитивная карта, будет пременена в конце кадра)
			int index = 0;
			for (int yi = y1i; yi <= y2i; ++yi) {
				for (int xi = x1i; xi <= x2i; ++xi) {
					if (xi < 0 || xi >= SceneSize || yi < 0 || yi >= SceneSize) {
						continue;
					}
					scene[xi][yi].smokeDensAdd += scene[x][y].smokeDens * coeffs[index++];
				}
			}
		}
	}

	// Применяем карту аддитивки и очищаем её
	for (int y = 0; y < SceneSize; ++y) {
		for (int x = 0; x < SceneSize; ++x) {
			scene[x][y].smokeDens += scene[x][y].smokeDensAdd;
			if (scene[x][y].smokeDens < 0) {
				exit_msg("smokeDens < 0 !");
			}
			scene[x][y].smokeDensAdd = 0;
		}
	}
}

//--------------------------------------------------------------------------------------------

//printf("%f", partAll);








//--------------------------------------------------------------------------------------------

void setSourseSmokeDensity()
{
	for (int y = 0; y < 10; ++y) {
		for (int x = 0; x < 10; ++x) {
			scene[x][100 + y].smokeDens = 0.3f;
		}
	}
}

//--------------------------------------------------------------------------------------------


void test1()
{
	// -------------
	//scene[0][199].smokeDens = 1;
	//scene[199][0].smokeDens = 0.6f;
	//scene[199][199].smokeDens = 0.3f;
	//saveSceneToBmp("smokeScene00.bmp");

	// -------------
	for (int y = 0; y < SceneSize; ++y) {
		for (int x = 0; x < SceneSize; ++x) {
			scene[x][y].speedX = 3.f;
		}
	}

	for (int i = 0; i < 100; ++i) {
		setSourseSmokeDensity();
		update();
		if (i % 5 == 0) {
			char number[4];
			number[0] = i / 100 + '0';
			number[1] = (i % 100) / 10 + '0';
			number[2] = (i % 10) + '0';
			number[3] = 0;
			std::string fname = std::string("frame") +(const char*)number +".bmp";
			saveSceneToBmp(fname);
		}
		printf("i=%d ", i);
	}
}


//--------------------------------------------------------------------------------------------

int main()
{
	test1();
}
