#include "pch.h"
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <algorithm>
#include "bmp.h"

//--------------------------------------------------------------------------------------------

double randDouble()
{
	return ((double)rand()) / RAND_MAX;
}

//--------------------------------------------------------------------------------------------

const int Scene2DSize = 200;
float Scale;
double SpeedSlowdown;

//--------------------------------------------------------------------------------------------

struct Cell2D
{
	float smokeDens = 0;
	float speedX = 0;
	float speedY = 0;

	float smokeDensAdd = 0;
	float speedXAdd = 0;
	float speedYAdd = 0;
};

//--------------------------------------------------------------------------------------------

Cell2D scene2D[Scene2DSize][Scene2DSize];

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

void save2DSceneToBmp(const std::string& fileName)
{
	uint8_t* bmpData = new uint8_t[Scene2DSize * Scene2DSize * 3];
	for (int y=0; y < Scene2DSize; ++y)
	{
		for (int x = 0; x < Scene2DSize; ++x)
		{
			uint8_t bright = std::min(int(scene2D[x][y].smokeDens * 255), 255);
			int pixelOffs = (y * Scene2DSize + x) * 3;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
		}
	}
	
	save_bmp24(fileName.c_str(), Scene2DSize, Scene2DSize, (const char *)bmpData);
	delete[] bmpData;
}

//--------------------------------------------------------------------------------------------

void update()
{
	std::vector<float> coeffs; // Коэффициенты зацепления зааффекченных клатов для нормализации

	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			// Расчитаем новые координаты углов текущей клетки после сдвига и расширения
			float x1 = (float)x;
			float y1 = (float)y;
			float x2 = x1 + 1.f;
			float y2 = y1 + 1.f;
			float xCenter = x1 + 0.5f;
			float yCenter = y1 + 0.5f;
			x1 = (x1 - xCenter) * Scale + xCenter + scene2D[x][y].speedX;
			y1 = (y1 - yCenter) * Scale + yCenter + scene2D[x][y].speedY;
			x2 = (x2 - xCenter) * Scale + xCenter + scene2D[x][y].speedX;
			y2 = (y2 - yCenter) * Scale + yCenter + scene2D[x][y].speedY;

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
			scene2D[x][y].smokeDensAdd -= scene2D[x][y].smokeDens;
			scene2D[x][y].speedXAdd -= scene2D[x][y].speedX;
			scene2D[x][y].speedYAdd -= scene2D[x][y].speedY;

			// Прибавляем плотность дыма и скорости текущей клетки ко всем заффекченным клеткам с нормированными коэффициентами перекрытия по клеткам периметра (аддитивная карта, будет пременена в конце кадра)
			int index = 0;
			for (int yi = y1i; yi <= y2i; ++yi) {
				for (int xi = x1i; xi <= x2i; ++xi) {
					if (xi < 0 || xi >= Scene2DSize || yi < 0 || yi >= Scene2DSize) {
						continue;
					}
					scene2D[xi][yi].smokeDensAdd += scene2D[x][y].smokeDens * coeffs[index];
					scene2D[xi][yi].speedXAdd += scene2D[x][y].speedX * coeffs[index] * SpeedSlowdown;
					scene2D[xi][yi].speedYAdd += scene2D[x][y].speedY * coeffs[index] * SpeedSlowdown;
					index++;
				}
			}
		}
	}

	// Применяем карту аддитивки и очищаем её
	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			scene2D[x][y].smokeDens += scene2D[x][y].smokeDensAdd;
			scene2D[x][y].speedX += scene2D[x][y].speedXAdd;
			scene2D[x][y].speedY += scene2D[x][y].speedYAdd;
			if (scene2D[x][y].smokeDens < 0) {
				exit_msg("smokeDens < 0 !");
			}
			scene2D[x][y].smokeDensAdd = 0;
			scene2D[x][y].speedXAdd = 0;
			scene2D[x][y].speedYAdd = 0;
		}
	}
}

//--------------------------------------------------------------------------------------------

void setSourseSmokeDensityAndSpeed1(int frame)
{
	for (int y = 0; y < 10; ++y) {
		for (int x = 0; x < 10; ++x) {
			if (frame < 200) {
				scene2D[x][100 + y].smokeDens = 0.3f;
			}
			scene2D[x][100 + y].speedX = 0.4f;
		}
	}

	for (int y = 0; y < 10; ++y) {
		for (int x = 0; x < 10; ++x) {
			scene2D[60+x][Scene2DSize-1-y].speedY = -1.f;
		}
	}
}

//--------------------------------------------------------------------------------------------

void test1()
{
	Scale = 1; // 1.6f;
	SpeedSlowdown = 1; // 0.999f;
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
			save2DSceneToBmp(fname);
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
				scene2D[x][y].smokeDens = randDouble()*0.6f;
			}
			scene2D[x][y].speedX = 0.5f;
		}
	}

	for (int y = 0; y < 120; ++y) {
		for (int x = 0; x < 10; ++x) {
			Cell2D& cell = scene2D[Scene2DSize - 1 - x][Scene2DSize - 1 - y];
			cell.speedX = -0.5f;
			if (randDouble() < 0.5)
				cell.smokeDens = 0;
			else
				cell.smokeDens = 0.6f;
		}
	}
}

//--------------------------------------------------------------------------------------------

void test2()
{
	Scale = 1.5f;
	SpeedSlowdown = 0.995f;
	for (int i = 0; i < 1000; ++i) {
		setSourseSmokeDensityAndSpeed2(i);
		update();
		if (i % 10 == 0) {
			char number[4];
			number[0] = i / 100 + '0';
			number[1] = (i % 100) / 10 + '0';
			number[2] = (i % 10) + '0';
			number[3] = 0;
			std::string fname = std::string("frame") + (const char*)number + ".bmp";
			save2DSceneToBmp(fname);
		}
		printf("i=%d ", i);
	}
}

//--------------------------------------------------------------------------------------------
// Ренедр
//--------------------------------------------------------------------------------------------

const float FarAway = 100000.f;
const int ScreenSize = 300; // Размер экрана в пикселах
const int SceneSize = 200;  // Размер сцены в единичных кубах
const float cameraZinit = -200; // Позиция камеры по z в системе координат сетки
const float MaxLightBright = 1000; // Максимальная яркость источника света
const double ScatterCoeff = 0.00002; // Коэффициент рассеивания тумана
const int SceneDrawNum = 400; // Сколько раз рендерим сцену

struct LIGHT_BOX
{
	LIGHT_BOX(float bright_, float x1_, float y1_, float z1_, float x2_, float y2_, float z2_) : bright(bright_), x1(x1_), y1(y1_), z1(z1_), x2(x2_), y2(y2_), z2(z2_) {}

	float bright = 0;
	float x1 = 0;
	float x2 = 0;
	float y1 = 0;
	float y2 = 0;
	float z1 = 0;
	float z2 = 0;
};

struct Cell
{
	float smokeDens = 0;
};


double screen[ScreenSize][ScreenSize];
std::vector<LIGHT_BOX> lights;
Cell scene[SceneSize][SceneSize][SceneSize];

//--------------------------------------------------------------------------------------------

inline float truncOneSide(float x)
{
	return (float)((int)((double)x + 1000) - 1000);
}

//--------------------------------------------------------------------------------------------


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
		return;
	}

	newX = truncOneSide(x);
	if (newX == x) {
		newX = truncOneSide(x) - 1.f;
	}
	float a = (newX - x) / dirX;
	newY = y + dirY*a;
	newZ = z + dirZ*a;
}

//--------------------------------------------------------------------------------------------

inline void intersectUp(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (fabs(dirY) < 0.001f) {
		return;
	}

	newY = truncOneSide(y);
	if (newY == y) {
		newY = truncOneSide(y) - 1.f;
	}
	float a = (newY - y) / dirY;
	newX = x + dirX * a;
	newZ = z + dirZ * a;
}

//--------------------------------------------------------------------------------------------

inline void intersectFront(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (fabs(dirZ) < 0.001f) {
		return;
	}

	newZ = truncOneSide(z);
	if (newZ == z) {
		newZ = truncOneSide(z) - 1.f;
	}
	float a = (newZ - z) / dirZ;
	newX = x + dirX * a;
	newY = y + dirY * a;
}

//--------------------------------------------------------------------------------------------

inline void intersectRight(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ)
{
	if (fabs(dirX) < 0.001f) {
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
		return;
	}

	newZ = (float)(((int)z) + 1);
	float a = (newZ - z) / dirZ;
	newX = x + dirX * a;
	newY = y + dirY * a;
}

//--------------------------------------------------------------------------------------------


void intersect(float x, float y, float z, float dirX, float dirY, float dirZ, float& newX, float& newY, float& newZ, int& cubeX, int& cubeY, int& cubeZ)
{
	float newX1 = FarAway;
	float newY1 = FarAway;
	float newZ1 = FarAway;
	float newX2 = FarAway;
	float newY2 = FarAway;
	float newZ2 = FarAway;
	float newX3 = FarAway;
	float newY3 = FarAway;
	float newZ3 = FarAway;
	if (dirX < 0) {
		if (dirY < 0) {
			if (dirZ < 0) {
				intersectLeft(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				intersectUp(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				intersectFront(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
			}
			else {
				intersectLeft(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				intersectUp(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				intersectBack(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
			}
		}
		else {
			if (dirZ < 0) {
				intersectLeft(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				intersectDown(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				intersectFront(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
			}
			else {
				intersectLeft(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				intersectDown(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				intersectBack(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
			}
		}
	}
	else {
		if (dirY < 0) {
			if (dirZ < 0) {
				intersectRight(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				intersectUp(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				intersectFront(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
			}
			else {
				intersectRight(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				intersectUp(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				intersectBack(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
			}
		}
		else {
			if (dirZ < 0) {
				intersectRight(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				intersectDown(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				intersectFront(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
			}
			else {
				intersectRight(x, y, z, dirX, dirY, dirZ, newX1, newY1, newZ1);
				intersectDown(x, y, z, dirX, dirY, dirZ, newX2, newY2, newZ2);
				intersectBack(x, y, z, dirX, dirY, dirZ, newX3, newY3, newZ3);
			}
		}
	}
	
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

	float centerX = (x + newX) * 0.5f;
	float centerY = (y + newY) * 0.5f;
	float centerZ = (z + newZ) * 0.5f;
	cubeX = ((int)((double)centerX + 10000)) - 10000;
	cubeY = ((int)((double)centerY + 10000)) - 10000;
	cubeZ = ((int)((double)centerZ + 10000)) - 10000;
}

//--------------------------------------------------------------------------------------------

void test3()
{
	float x = 10.15f;
	float y = 6.95f;
	float z = 3.f;
	float dirX = -0.1f;
	float dirY = -0.1f;
	float dirZ = -1.f;

	float newX = 0;
	float newY = 0;
	float newZ = 0;
	int cubeX = 0;
	int cubeY = 0;
	int cubeZ = 0;

	for (int i = 0; i < 4; ++i) {
		intersect(x, y, z, dirX, dirY, dirZ, newX, newY, newZ, cubeX, cubeY, cubeZ);
		printf("%f %f %f\n", newX, newY, newZ);
		x = newX;
		y = newY;
		z = newZ;
	}
}

//--------------------------------------------------------------------------------------------
// Определить цвет пиксела и записать его в буфер screen
//--------------------------------------------------------------------------------------------

void renderPixel(int xi, int yi, float x, float y, float z, float dirX, float dirY, float dirZ)
{
	double lightDrop = 0;
	double scatterProb = 0;

	while(true) {
		if (x < -1 || x > SceneSize + 1 || y < -1 || y > SceneSize + 1 || z < -1 || z > SceneSize + 1) {
			return;
		}
		for (const auto& light : lights) {
			if (x > light.x1 && x < light.x2 && y > light.y1 && y < light.y2 && z > light.z1 && z < light.z2) {
				screen[xi][yi] += std::max(light.bright - lightDrop, 0.);
				return;
			}
		}

		float newX = 0;
		float newY = 0;
		float newZ = 0;
		int cubeX = 0;
		int cubeY = 0;
		int cubeZ = 0;
		intersect(x, y, z, dirX, dirY, dirZ, newX, newY, newZ, cubeX, cubeY, cubeZ);
		float dist = sqrtf((x, y, z, newX, newY, newZ));
		x = newX;
		y = newY;
		z = newZ;

		float smokeDens = 0;
		if (cubeX >= 0 && cubeX < SceneSize && cubeY >= 0 && cubeY < SceneSize && cubeZ >= 0 && cubeZ < SceneSize) {
			smokeDens = scene[cubeX][cubeY][cubeZ].smokeDens;
		}

		lightDrop += smokeDens * dist * 0.2f;
		if (lightDrop > MaxLightBright) {
			return;
		}

		double curScatterProb = smokeDens * dist * ScatterCoeff;
		scatterProb = 1. - (1. - scatterProb) * (1. - curScatterProb);
		if (randDouble() < scatterProb) {
			scatterProb = 0;
			dirX = (float)(randDouble() * 2.f - 1.f);
			dirY = (float)(randDouble() * 2.f - 1.f);
			dirZ = (float)(randDouble() * 2.f - 1.f);
		}
	}
}

//--------------------------------------------------------------------------------------------
// Рендерить сцену в буфер screen
//--------------------------------------------------------------------------------------------

void renderScene()
{
	float cameraX = SceneSize / 2.f;
	float cameraY = SceneSize / 2.f;
	float cameraZ = cameraZinit;

	double ratio = (double)SceneSize / (double)ScreenSize;
	for (int yi = 0; yi < ScreenSize; ++yi)
	{
		for (int xi = 0; xi < ScreenSize; ++xi)
		{
			float x = (float)((((double)xi) + 0.5f) * ratio);
			float y = (float)((((double)yi) + 0.5f) * ratio);
			float z = 0;
			float dirX = x - cameraX;
			float dirY = y - cameraY;
			float dirZ = z - cameraZ;

			renderPixel(xi, yi, x, y, z, dirX, dirY, dirZ);
		}
	}
}

//--------------------------------------------------------------------------------------------

void saveSceneToBmp(const std::string& fileName)
{
	uint8_t* bmpData = new uint8_t[ScreenSize * ScreenSize * 3];
	for (int y = 0; y < ScreenSize; ++y)
	{
		for (int x = 0; x < ScreenSize; ++x)
		{
			uint8_t bright = (uint8_t)(std::min(screen[x][y] / SceneDrawNum, 255.));
			int pixelOffs = (y * ScreenSize + x) * 3;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
		}
	}

	save_bmp24(fileName.c_str(), ScreenSize, ScreenSize, (const char *)bmpData);
	delete[] bmpData;
}

//--------------------------------------------------------------------------------------------

void test4()
{
	lights.push_back(LIGHT_BOX(MaxLightBright, 10, 20, 150, 30, 50, 200));
	lights.push_back(LIGHT_BOX(MaxLightBright, 180, 170, 0, 200, 200, 200));

	for (int z = 30; z < 170; ++z) {
		for (int y= 30; y < 170; ++y) {
			for (int x = 30; x < 170; ++x) {
				scene[x][y][z].smokeDens = 1.f;
			}
		}
	}

	for (int n=0; n < SceneDrawNum; ++n) {
		renderScene();
		printf("%d\n", n);
	}

	saveSceneToBmp("3dScene.bmp");
}

int main()
{
	test2();
//	test4();
}
