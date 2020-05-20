#include "pch.h"
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <algorithm>
#include "bmp.h"

//--------------------------------------------------------------------------------------------

const int SceneSize = 200;
float Scale = 1.6f;
float SpeedSlowdown = 0.99f;

//--------------------------------------------------------------------------------------------

struct Cell
{
	float smokeDens = 0;
	float speedX = 0;
	float speedY = 0;

	float smokeDensAdd = 0;
	float speedXAdd = 0;
	float speedYAdd = 0;
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
			uint8_t bright = std::min(int(scene[x][y].smokeDens * 128), 128);
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

			// Вычитаем плотность дыма и скорости текущей клетки из своей же клетки (аддитивная карта, будет пременена в конце кадра)
			scene[x][y].smokeDensAdd -= scene[x][y].smokeDens;
			scene[x][y].speedXAdd -= scene[x][y].speedX;
			scene[x][y].speedYAdd -= scene[x][y].speedY;

			// Прибавляем плотность дыма и скорости текущей клетки ко всем заффекченным клеткам с нормированными коэффициентами перекрытия по клеткам периметра (аддитивная карта, будет пременена в конце кадра)
			int index = 0;
			for (int yi = y1i; yi <= y2i; ++yi) {
				for (int xi = x1i; xi <= x2i; ++xi) {
					if (xi < 0 || xi >= SceneSize || yi < 0 || yi >= SceneSize) {
						continue;
					}
					scene[xi][yi].smokeDensAdd += scene[x][y].smokeDens * coeffs[index];
					scene[xi][yi].speedXAdd += scene[x][y].speedX * coeffs[index] * SpeedSlowdown;
					scene[xi][yi].speedYAdd += scene[x][y].speedY * coeffs[index] * SpeedSlowdown;
					index++;
				}
			}
		}
	}

	// Применяем карту аддитивки и очищаем её
	for (int y = 0; y < SceneSize; ++y) {
		for (int x = 0; x < SceneSize; ++x) {
			scene[x][y].smokeDens += scene[x][y].smokeDensAdd;
			scene[x][y].speedX += scene[x][y].speedXAdd;
			scene[x][y].speedY += scene[x][y].speedYAdd;
			if (scene[x][y].smokeDens < 0) {
				exit_msg("smokeDens < 0 !");
			}
			scene[x][y].smokeDensAdd = 0;
			scene[x][y].speedXAdd = 0;
			scene[x][y].speedYAdd = 0;
		}
	}
}

//--------------------------------------------------------------------------------------------

void setSourseSmokeDensityAndSpeed1(int frame)
{
	for (int y = 0; y < 10; ++y) {
		for (int x = 0; x < 10; ++x) {
			if (frame < 200) {
				scene[x][100 + y].smokeDens = 0.3f;
			}
			scene[x][100 + y].speedX = 0.4f;
		}
	}

	for (int y = 0; y < 10; ++y) {
		for (int x = 0; x < 10; ++x) {
			scene[60+x][SceneSize-1-y].speedY = -1.f;
		}
	}
}

//--------------------------------------------------------------------------------------------

void test1()
{
	Scale = 1.6f;
	SpeedSlowdown = 0.99f;
	for (int i = 0; i < 1000; ++i) {
		setSourseSmokeDensityAndSpeed1(i);
		update();
		if (i % 25 == 0) {
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

void setSourseSmokeDensityAndSpeed2(int frame)
{
	for (int y = 0; y < 120; ++y) {
		for (int x = 0; x < 10; ++x) {
			if (frame < 200) {
				scene[x][y].smokeDens = 0.3f;
			}
			scene[x][y].speedX = 0.5f;
		}
	}

	for (int y = 0; y < 120; ++y) {
		for (int x = 0; x < 10; ++x) {
			scene[SceneSize - 1 - x][SceneSize - 1 - y].speedX = -0.5f;
		}
	}
}

//--------------------------------------------------------------------------------------------

void test2()
{
	Scale = 1.6f;
	SpeedSlowdown = 0.99f;
	for (int i = 0; i < 1000; ++i) {
		setSourseSmokeDensityAndSpeed2(i);
		update();
		if (i % 25 == 0) {
			char number[4];
			number[0] = i / 100 + '0';
			number[1] = (i % 100) / 10 + '0';
			number[2] = (i % 10) + '0';
			number[3] = 0;
			std::string fname = std::string("frame") + (const char*)number + ".bmp";
			saveSceneToBmp(fname);
		}
		printf("i=%d ", i);
	}
}

//--------------------------------------------------------------------------------------------
// Ренедр
//--------------------------------------------------------------------------------------------

const float FarAway = 100000.f;

inline float distSq(float x1, float y1, float z1, float x2, float y2, float z2)
{
	float dx = x1 - x2;
	float dy = y1 - y2;
	float dz = z1 - z2;
	return dx * dx + dy * dy + dz * dz;
}

//--------------------------------------------------------------------------------------------

inline void intersectLeft(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (fabs(dirX) < 0.001f) {
		newX = FarAway;
		newY = FarAway;
		newZ = FarAway;
		return;
	}

	newX = (float)(int)x;
	if (newX == x) {
		newX = (float)(((int)x) - 1);
	}
	float a = (newX - x) / dirX;
	newY = y + dirY*a;
	newZ = z + dirZ*a;
}

//--------------------------------------------------------------------------------------------

inline void intersectUp(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (fabs(dirY) < 0.001f) {
		newX = FarAway;
		newY = FarAway;
		newZ = FarAway;
		return;
	}

	newY = (float)(int)y;
	if (newY == y) {
		newY = (float)(((int)y) - 1);
	}
	float a = (newY - y) / dirY;
	newX = x + dirX * a;
	newZ = z + dirZ * a;
}

//--------------------------------------------------------------------------------------------

inline void intersectFront(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (fabs(dirZ) < 0.001f) {
		newX = FarAway;
		newY = FarAway;
		newZ = FarAway;
		return;
	}

	newZ = (float)(int)z;
	if (newZ == z) {
		newZ = (float)(((int)z) - 1);
	}
	float a = (newZ - z) / dirZ;
	newX = x + dirX * a;
	newY = y + dirY * a;
}

//--------------------------------------------------------------------------------------------

inline void intersectRight(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (fabs(dirX) < 0.001f) {
		newX = FarAway;
		newY = FarAway;
		newZ = FarAway;
		return;
	}

	newX = (float)(((int)x) + 1);
	float a = (newX - x) / dirX;
	newY = y + dirY * a;
	newZ = z + dirZ * a;
}

//--------------------------------------------------------------------------------------------

inline void intersectDown(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (fabs(dirY) < 0.001f) {
		newX = FarAway;
		newY = FarAway;
		newZ = FarAway;
		return;
	}

	newY = (float)(((int)y) + 1);
	float a = (newY - y) / dirY;
	newX = x + dirX * a;
	newZ = z + dirZ * a;
}

//--------------------------------------------------------------------------------------------

inline void intersectBack(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (fabs(dirZ) < 0.001f) {
		newX = FarAway;
		newY = FarAway;
		newZ = FarAway;
		return;
	}

	newZ = (float)(((int)z) + 1);
	float a = (newZ - z) / dirZ;
	newX = x + dirX * a;
	newY = y + dirY * a;
}

//--------------------------------------------------------------------------------------------


void intersect(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (dirX < 0) {
		if (dirY < 0) {
			if (dirZ < 0) {
				float newX1 = 0;
				float newY1 = 0;
				float newZ1 = 0;
				intersectLeft(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				float newX2 = 0;
				float newY2 = 0;
				float newZ2 = 0;
				intersectUp(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				float newX3 = 0;
				float newY3 = 0;
				float newZ3 = 0;
				intersectFront(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
				float dist1 = distSq(x, y, z, newX1, newY1, newZ1);
				float dist2 = distSq(x, y, z, newX2, newY2, newZ2);
				float dist3 = distSq(x, y, z, newX3, newY3, newZ3);
				if (dist1 < dist2) {
					if (dist1 < dist3) {
						newX = newX1;
						newY = newY1;
						newZ = newZ1;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
				else {
					if (dist2 < dist3) {
						newX = newX2;
						newY = newY2;
						newZ = newZ2;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
			}
			else {
				float newX1 = 0;
				float newY1 = 0;
				float newZ1 = 0;
				intersectLeft(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				float newX2 = 0;
				float newY2 = 0;
				float newZ2 = 0;
				intersectUp(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				float newX3 = 0;
				float newY3 = 0;
				float newZ3 = 0;
				intersectBack(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
				float dist1 = distSq(x, y, z, newX1, newY1, newZ1);
				float dist2 = distSq(x, y, z, newX2, newY2, newZ2);
				float dist3 = distSq(x, y, z, newX3, newY3, newZ3);
				if (dist1 < dist2) {
					if (dist1 < dist3) {
						newX = newX1;
						newY = newY1;
						newZ = newZ1;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
				else {
					if (dist2 < dist3) {
						newX = newX2;
						newY = newY2;
						newZ = newZ2;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
			}
		}
		else {
			if (dirZ < 0) {
				float newX1 = 0;
				float newY1 = 0;
				float newZ1 = 0;
				intersectLeft(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				float newX2 = 0;
				float newY2 = 0;
				float newZ2 = 0;
				intersectDown(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				float newX3 = 0;
				float newY3 = 0;
				float newZ3 = 0;
				intersectFront(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
				float dist1 = distSq(x, y, z, newX1, newY1, newZ1);
				float dist2 = distSq(x, y, z, newX2, newY2, newZ2);
				float dist3 = distSq(x, y, z, newX3, newY3, newZ3);
				if (dist1 < dist2) {
					if (dist1 < dist3) {
						newX = newX1;
						newY = newY1;
						newZ = newZ1;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
				else {
					if (dist2 < dist3) {
						newX = newX2;
						newY = newY2;
						newZ = newZ2;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
			}
			else {
				float newX1 = 0;
				float newY1 = 0;
				float newZ1 = 0;
				intersectLeft(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				float newX2 = 0;
				float newY2 = 0;
				float newZ2 = 0;
				intersectDown(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				float newX3 = 0;
				float newY3 = 0;
				float newZ3 = 0;
				intersectBack(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
				float dist1 = distSq(x, y, z, newX1, newY1, newZ1);
				float dist2 = distSq(x, y, z, newX2, newY2, newZ2);
				float dist3 = distSq(x, y, z, newX3, newY3, newZ3);
				if (dist1 < dist2) {
					if (dist1 < dist3) {
						newX = newX1;
						newY = newY1;
						newZ = newZ1;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
				else {
					if (dist2 < dist3) {
						newX = newX2;
						newY = newY2;
						newZ = newZ2;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
			}
		}
	}
	else {
		if (dirY < 0) {
			if (dirZ < 0) {
				float newX1 = 0;
				float newY1 = 0;
				float newZ1 = 0;
				intersectRight(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				float newX2 = 0;
				float newY2 = 0;
				float newZ2 = 0;
				intersectUp(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				float newX3 = 0;
				float newY3 = 0;
				float newZ3 = 0;
				intersectFront(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
				float dist1 = distSq(x, y, z, newX1, newY1, newZ1);
				float dist2 = distSq(x, y, z, newX2, newY2, newZ2);
				float dist3 = distSq(x, y, z, newX3, newY3, newZ3);
				if (dist1 < dist2) {
					if (dist1 < dist3) {
						newX = newX1;
						newY = newY1;
						newZ = newZ1;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
				else {
					if (dist2 < dist3) {
						newX = newX2;
						newY = newY2;
						newZ = newZ2;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
			}
			else {
				float newX1 = 0;
				float newY1 = 0;
				float newZ1 = 0;
				intersectRight(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				float newX2 = 0;
				float newY2 = 0;
				float newZ2 = 0;
				intersectUp(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				float newX3 = 0;
				float newY3 = 0;
				float newZ3 = 0;
				intersectBack(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
				float dist1 = distSq(x, y, z, newX1, newY1, newZ1);
				float dist2 = distSq(x, y, z, newX2, newY2, newZ2);
				float dist3 = distSq(x, y, z, newX3, newY3, newZ3);
				if (dist1 < dist2) {
					if (dist1 < dist3) {
						newX = newX1;
						newY = newY1;
						newZ = newZ1;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
				else {
					if (dist2 < dist3) {
						newX = newX2;
						newY = newY2;
						newZ = newZ2;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
			}
		}
		else {
			if (dirZ < 0) {
				float newX1 = 0;
				float newY1 = 0;
				float newZ1 = 0;
				intersectRight(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				float newX2 = 0;
				float newY2 = 0;
				float newZ2 = 0;
				intersectDown(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				float newX3 = 0;
				float newY3 = 0;
				float newZ3 = 0;
				intersectFront(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
				float dist1 = distSq(x, y, z, newX1, newY1, newZ1);
				float dist2 = distSq(x, y, z, newX2, newY2, newZ2);
				float dist3 = distSq(x, y, z, newX3, newY3, newZ3);
				if (dist1 < dist2) {
					if (dist1 < dist3) {
						newX = newX1;
						newY = newY1;
						newZ = newZ1;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
				else {
					if (dist2 < dist3) {
						newX = newX2;
						newY = newY2;
						newZ = newZ2;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
			}
			else {
				float newX1 = 0;
				float newY1 = 0;
				float newZ1 = 0;
				intersectRight(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				float newX2 = 0;
				float newY2 = 0;
				float newZ2 = 0;
				intersectDown(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				float newX3 = 0;
				float newY3 = 0;
				float newZ3 = 0;
				intersectBack(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
				float dist1 = distSq(x, y, z, newX1, newY1, newZ1);
				float dist2 = distSq(x, y, z, newX2, newY2, newZ2);
				float dist3 = distSq(x, y, z, newX3, newY3, newZ3);
				if (dist1 < dist2) {
					if (dist1 < dist3) {
						newX = newX1;
						newY = newY1;
						newZ = newZ1;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
				else {
					if (dist2 < dist3) {
						newX = newX2;
						newY = newY2;
						newZ = newZ2;
					}
					else {
						newX = newX3;
						newY = newY3;
						newZ = newZ3;
					}
				}
			}
		}
	}





}

//--------------------------------------------------------------------------------------------

void test3()
{
	float x = 10.9f;
	float y = 6.f;
	float z = 3.8f;
	float dirX = -1.f;
	float dirY = -1.f;
	float dirZ = -1.f;

	float newX = 0;
	float newY = 0;
	float newZ = 0;

	for (int i = 0; i < 1; ++i) {
		intersect(x, y, z, dirX, dirY, dirZ, newX, newY, newZ);
		printf("%f %f %f\n", newX, newY, newZ);
		x = newX;
		y = newY;
		z = newZ;
	}
}

//--------------------------------------------------------------------------------------------


int main()
{
	test3();
}
