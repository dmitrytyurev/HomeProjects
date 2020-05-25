#include "pch.h"
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <algorithm>
#include "bmp.h"

//--------------------------------------------------------------------------------------------
//FILE* f = fopen("WaveTable.txt", "wt");
//fprintf(f, "%f\n", waveTable[i]);
//fclose(f);
//--------------------------------------------------------------------------------------------

const float PI = 3.14159f;
const int Scene2DSize = 200;
const int WaveTableSize = 1000;
float randomSpeedRotateAmpl;  // Амплитуда рандомного вращения скоростей
float randomSpeedRotateSpeed;  // Скорость рандомного вращения скоростей
float randomSpeedAccelerateAml; // Амплитуда андомного изменения скоростей
//--------------------------------------------------------------------------------------------

float waveTable[WaveTableSize];
float Scale;
double SpeedSlowdown;

//--------------------------------------------------------------------------------------------

struct Cell2D
{
	float smokeDens = 0;
	float airDens = 0;
	float speedX = 0;
	float speedY = 0;
	float avgSpeedX = 0;
	float avgSpeedY = 0;

	float smokeDensAdd = 0;
	float airDensAdd = 0;
	float speedXAdd = 0;
	float speedYAdd = 0;
	int waveTableRandomOffset = 0;
	int avgWaveTableRandomOffset = 0;
};

//--------------------------------------------------------------------------------------------

Cell2D scene2D[Scene2DSize][Scene2DSize];

//--------------------------------------------------------------------------------------------

struct ObjectToPlace
{
	ObjectToPlace(int x_, int y_, int srcBuferIndex_, float newOuterRadius_) : x(x_), y(y_), srcBuferIndex(srcBuferIndex_), newOuterRadius(newOuterRadius_) {}
	float calcInnerRadius() const;

	int x = 0;                // Позиция центра, куда 
	int y = 0;                //                      будем ставить объект

	int srcBuferIndex = 0;    // Индекс Src-буфера, из которого возьмём объект
	float newOuterRadius = 0; // Новый внешний радиус, с которым будет установлен объект
};
std::vector<ObjectToPlace> objects;

//--------------------------------------------------------------------------------------------

struct Buffer
{
	void clear();
	void fillParameters();

	float cells[Scene2DSize][Scene2DSize];
	int centerX = 0;
	int centerY = 0;
	float innerRadius = 0;
	float outerRadius = 0;
};
const int BuffersNum = 10;

Buffer srcBuffers[BuffersNum];
Buffer dstBuffers[BuffersNum];

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

int rand(int min, int max)
{
	return rand() * (max - min + 1) / (RAND_MAX + 1) + min;
}

//--------------------------------------------------------------------------------------------

float  randf(float min, float max)
{
	return rand() * (max - min) / RAND_MAX  + min;
}

//--------------------------------------------------------------------------------------------

inline float calcDist(float x1, float y1, float x2, float y2)
{
	float dx = x1 - x2;
	float dy = y1 - y2;
	return sqrt(dx * dx + dy * dy);
}

//--------------------------------------------------------------------------------------------

void turnPoint(float x, float y, double angle, float& newX, float& newY)
{
	double s = sin(angle);
	double c = cos(angle);
	newX = (float)(x * c - y * s);
	newY = (float)(x * s + y * c);
}

//--------------------------------------------------------------------------------------------

void initWaveTable()
{
	for (int i = 0; i < WaveTableSize; ++i) {
		float angle = ((float)i) / WaveTableSize * 2 * PI;
		waveTable[i] = sin(angle) + sin(angle * 2) + sin(angle * 3) + sin(angle * 4);
	}
}

//--------------------------------------------------------------------------------------------

void save2DSceneToBmp(const std::string& fileName)
{
	uint8_t* bmpData = new uint8_t[Scene2DSize * Scene2DSize * 3];
	for (int y=0; y < Scene2DSize; ++y)
	{
		for (int x = 0; x < Scene2DSize; ++x)
		{
			uint8_t bright = std::min(int(scene2D[x][y].smokeDens * 255), 255);          // Плотности 2d-сцены
			//uint8_t bright = std::min(fabs(scene2D[x][y].speedX * 128), 255.f);          // Карта скоростей 2d-сцены
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

void saveDstBufToBmp(const std::string& fileName, int bufIndex)
{
	uint8_t* bmpData = new uint8_t[Scene2DSize * Scene2DSize * 3];
	for (int y = 0; y < Scene2DSize; ++y)
	{
		for (int x = 0; x < Scene2DSize; ++x)
		{
			uint8_t bright = std::min(int(dstBuffers[bufIndex].cells[x][y] * 255), 255);            // Dst буфер
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

int xOffs[] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
int yOffs[] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };

void caclAirDensGradient(int xi, int yi, float& dx, float& dy)
{
	if (xi <= 0 || xi >= Scene2DSize - 1 || yi <= 0 || yi >= Scene2DSize - 1) {
		dx = 0;
		dy = 0;
		return;
	}

	dx = 0;
	dy = 0;
	for (int n1 = 0; n1 < 9; ++n1)
	{
		for (int n2 = n1 + 1; n2 < 9; ++n2)
		{
			float airDensDelta = scene2D[xi + xOffs[n1]][yi + yOffs[n1]].airDens - scene2D[xi + xOffs[n2]][yi + yOffs[n2]].airDens;
			dx = (xOffs[n2] - xOffs[n1]) * airDensDelta;
			dy = (yOffs[n2] - yOffs[n1]) * airDensDelta;
		}
	}
}

//--------------------------------------------------------------------------------------------

void update2dScene(int stepN)
{
	std::vector<float> coeffs; // Коэффициенты зацепления зааффекченных клатов для нормализации

	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			Cell2D& cell = scene2D[x][y];
			// Расчитаем новые координаты углов текущей клетки после сдвига и расширения
			float x1 = (float)x;
			float y1 = (float)y;
			float x2 = x1 + 1.f;
			float y2 = y1 + 1.f;

			//float xCenter = x1 + 0.5f;
			//float yCenter = y1 + 0.5f;
			//x1 = (x1 - xCenter) * Scale + xCenter + cell.speedX;
			//y1 = (y1 - yCenter) * Scale + yCenter + cell.speedY;
			//x2 = (x2 - xCenter) * Scale + xCenter + cell.speedX;
			//y2 = (y2 - yCenter) * Scale + yCenter + cell.speedY;




			// Сначала расширяем
			float xCenter = x1 + 0.5f;
			float yCenter = y1 + 0.5f;
			x1 = (x1 - xCenter) * Scale + xCenter;
			y1 = (y1 - yCenter) * Scale + yCenter;
			x2 = (x2 - xCenter) * Scale + xCenter;
			y2 = (y2 - yCenter) * Scale + yCenter;

			// Корретируем новую позицию, чтобы из-за расширения не зацепить клетки позади по ходу движения
			if (fabs(cell.speedX) > 0.001f || fabs(cell.speedY) > 0.001f) {
				float delta = (float)x - x1; // На сколько увеличенная клетка вылазит за пределы старой клетки (с одной стороны)
				float comX = 0;
				float comY = 0;
				if (fabs(cell.speedX) < fabs(cell.speedY)) {
					float k = fabs(cell.speedX / cell.speedY);
					comY = cell.speedY < 0 ? -delta : delta;
					comX = cell.speedX < 0 ? -delta * k : delta * k;
				}
				else {
					float k = fabs(cell.speedY / cell.speedX);
					comX = cell.speedX < 0 ? -delta : delta;
					comY = cell.speedY < 0 ? -delta * k : delta * k;
				}
				x1 += comX;
				x2 += comX;
				y1 += comY;
				y2 += comY;
			}

			// Сдвиг клиенти по вектору движения
			x1 += cell.speedX;
			y1 += cell.speedY;
			x2 += cell.speedX;
			y2 += cell.speedY;





			// Расчитаем какие клетки (и с каким весом) цепляет сдвинутая и расширенная текущая клетка
			int x1i = ((int)(x1 + 1000.)) - 1000;
			int y1i = ((int)(y1 + 1000.)) - 1000;
			int x2i = ((int)(x2 + 1000.)) - 1000;
			int y2i = ((int)(y2 + 1000.)) - 1000;

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
			cell.smokeDensAdd -= cell.smokeDens;
			cell.airDensAdd -= cell.airDens;
			cell.speedXAdd -= cell.speedX;
			cell.speedYAdd -= cell.speedY;

			// Прибавляем плотность дыма и скорости текущей клетки ко всем зааффекченным клеткам с нормированными коэффициентами перекрытия по клеткам периметра (аддитивная карта, будет пременена в конце кадра)
			int index = 0;
			for (int yi = y1i; yi <= y2i; ++yi) {
				for (int xi = x1i; xi <= x2i; ++xi) {
					if (xi < 0 || xi >= Scene2DSize || yi < 0 || yi >= Scene2DSize) {
						continue;
					}
					scene2D[xi][yi].smokeDensAdd += cell.smokeDens * coeffs[index];
					scene2D[xi][yi].airDensAdd += cell.airDens * coeffs[index];
					scene2D[xi][yi].speedXAdd += cell.speedX * coeffs[index];
					scene2D[xi][yi].speedYAdd += cell.speedY * coeffs[index];
					index++;
				}
			}
		}
	}
	
	// Применяем карту аддитивки и очищаем её
	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			Cell2D& cell = scene2D[x][y];

			cell.smokeDens += cell.smokeDensAdd;
			cell.airDens += cell.airDensAdd;
			cell.speedX += cell.speedXAdd;
			cell.speedY += cell.speedYAdd;
			cell.speedX = (float)(cell.speedX * SpeedSlowdown);
			cell.speedY = (float)(cell.speedY * SpeedSlowdown);

			if (cell.speedX > 1) cell.speedX = 1;
			if (cell.speedX < -1) cell.speedX = -1;
			if (cell.speedY > 1) cell.speedY = 1;
			if (cell.speedY < -1) cell.speedY = -1;

			if (cell.smokeDens < 0) {
				//printf("smokeDens = %f\n", cell.smokeDens);
				cell.smokeDens = 0;
			}
			cell.smokeDensAdd = 0;
			cell.airDensAdd = 0;
			cell.speedXAdd = 0;
			cell.speedYAdd = 0;
		}
	}

	// Меняем поле скорости от давления
	//for (int y = 1; y < Scene2DSize-1; ++y) {
	//	for (int x = 1; x < Scene2DSize - 1; ++x) {
	//		Cell2D& cell = scene2D[x][y];
	//		float dx = 0;
	//		float dy = 0;
	//		caclAirDensGradient(x, y, dx, dy);
	//		cell.speedX += dx * 0.1f;
	//		cell.speedY += dy * 0.1f;
	//	}
	//}

	 //Поворачиваем рандомно немного вектора скорости и меняем их скорость
	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			Cell2D& cell = scene2D[x][y];
			float scalarSpeed = sqrt(cell.speedX*cell.speedX + cell.speedY*cell.speedY);
			int index = (cell.waveTableRandomOffset + int(stepN * randomSpeedRotateSpeed * scalarSpeed * 3)) % WaveTableSize;
			double angle = waveTable[index] * randomSpeedRotateAmpl;

			turnPoint(cell.speedX, cell.speedY, angle, cell.speedX, cell.speedY);
			cell.speedX *= (1 + waveTable[index] * randomSpeedAccelerateAml);
			cell.speedY *= (1 + waveTable[index] * randomSpeedAccelerateAml);
		}
	}

	 // Усредняем поле скоростей, чтобы не накапливались эффекты положительной обратной связи
	for (int y = 1; y < Scene2DSize-1; ++y) {
		for (int x = 1; x < Scene2DSize - 1; ++x) {
			scene2D[x][y].avgSpeedX = (scene2D[x][y].speedX + scene2D[x - 1][y].speedX + scene2D[x + 1][y].speedX + scene2D[x][y - 1].speedX + scene2D[x][y + 1].speedX + scene2D[x - 1][y - 1].speedX + scene2D[x + 1][y - 1].speedX + scene2D[x - 1][y + 1].speedX + scene2D[x + 1][y + 1].speedX) / 9;
			scene2D[x][y].avgSpeedY = (scene2D[x][y].speedY + scene2D[x - 1][y].speedY + scene2D[x + 1][y].speedY + scene2D[x][y - 1].speedY + scene2D[x][y + 1].speedY + scene2D[x - 1][y - 1].speedY + scene2D[x + 1][y - 1].speedY + scene2D[x - 1][y + 1].speedY + scene2D[x + 1][y + 1].speedY) / 9;
		}
	}
	for (int y = 1; y < Scene2DSize - 1; ++y) {
		for (int x = 1; x < Scene2DSize - 1; ++x) {
			scene2D[x][y].speedX = scene2D[x][y].avgSpeedX;
			scene2D[x][y].speedY = scene2D[x][y].avgSpeedY;
		}
	}


}

//--------------------------------------------------------------------------------------------

void setSourseSmokeDensityAndSpeed2(int frame)
{
	for (int y = 0; y < 120; ++y) {
		for (int x = 0; x < 10; ++x) {
			Cell2D& cell = scene2D[x][y];
			cell.speedX = 0.5f;
			cell.airDens = 0.1f;
			if (frame < 100) {
				cell.smokeDens = randf(0, 0.6f);
			}
			else {
				cell.smokeDens = 0;
			}
		}
	}

	for (int y = 0; y < 120; ++y) {
		for (int x = 0; x < 10; ++x) {
			Cell2D& cell = scene2D[Scene2DSize - 1 - x][Scene2DSize - 1 - y];
			cell.speedX = -0.5f;
			cell.airDens = 0.1f;

			if (frame < 100) {
				if (randf(0.f, 1.f) < 0.5)
					cell.smokeDens = 0;
				else
					cell.smokeDens = 0.6f;
			}
			else {
				cell.smokeDens = 0;
			}
		}
	}
}

//--------------------------------------------------------------------------------------------

void test2Render2dAnimation()
{
	Scale = 1.02f; // 1.005f; // 1.002f; // 1.5f;
	SpeedSlowdown = 0.999f;
	randomSpeedRotateAmpl = 0.05f;
	randomSpeedRotateSpeed = 0.3f;
	randomSpeedAccelerateAml = 0;

	initWaveTable();

	//for (int y = 0; y < Scene2DSize; ++y) {
	//	for (int x = 0; x < Scene2DSize; ++x) {
	//		scene2D[x][y].airDens = 0.1f;
	//	}
	//}

	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			scene2D[x][y].waveTableRandomOffset = rand(0, 999);
		}
	}

	// Разблурить рандомные оффсеты 
	for (int y = 1; y < Scene2DSize - 1; ++y) {
		for (int x = 1; x < Scene2DSize - 1; ++x) {
			scene2D[x][y].avgWaveTableRandomOffset = (scene2D[x][y].waveTableRandomOffset + scene2D[x - 1][y].waveTableRandomOffset + scene2D[x + 1][y].waveTableRandomOffset + scene2D[x][y - 1].waveTableRandomOffset + scene2D[x][y + 1].waveTableRandomOffset + scene2D[x - 1][y - 1].waveTableRandomOffset + scene2D[x + 1][y - 1].waveTableRandomOffset + scene2D[x - 1][y + 1].waveTableRandomOffset + scene2D[x + 1][y + 1].waveTableRandomOffset) / 9;
		}
	}
	for (int y = 1; y < Scene2DSize - 1; ++y) {
		for (int x = 1; x < Scene2DSize - 1; ++x) {
			scene2D[x][y].waveTableRandomOffset = scene2D[x][y].avgWaveTableRandomOffset;
		}
	}


	for (int i = 0; i < 1000; ++i) {
		setSourseSmokeDensityAndSpeed2(i);
		update2dScene(i);
		if (i % 1 == 0) {
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

void Buffer::clear()
{
	centerX = 0;
	centerY = 0;
	innerRadius = 0;
	outerRadius = 0;
	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			cells[x][y] = 0;
		}
	}
}

//--------------------------------------------------------------------------------------------

void Buffer::fillParameters()
{
	// Найти описанный прямоугольник вокруг картинки в cells
	int x1 = 0;
	int x2 = 0;
	int y1 = 0;
	int y2 = 0;
	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			if (cells[x][y] > 0) {
				y1 = y;
				goto m1;
			}
		}
	}
m1:	for (int y = Scene2DSize-1; y >= 0; --y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			if (cells[x][y] > 0) {
				y2 = y;
				goto m2;
			}
		}
	}
m2:	for (int x = 0; x < Scene2DSize; ++x) {
		for (int y = 0; y < Scene2DSize; ++y) {
			if (cells[x][y] > 0) {
				x1 = x;
				goto m3;
			}
		}
	}
m3:	for (int x = Scene2DSize - 1; x >= 0; --x) {
		for (int y = 0; y < Scene2DSize; ++y) {
			if (cells[x][y] > 0) {
				x2 = x;
				goto m4;
			}
		}
	}
m4: centerX = (x1 + x2) / 2;
    centerY = (y1 + y2) / 2;

	// Расчитать innerRadius, outerRadius
	innerRadius = FLT_MAX;
	outerRadius = 0;
	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			float dist = calcDist((float)x, (float)y, (float)centerX, (float)centerY);
			if (cells[x][y] == 0) {
				if (dist < innerRadius) {
					innerRadius = dist;
				}
			}
			if (cells[x][y] > 0) {
				if (dist > outerRadius) {
					outerRadius = dist;
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------

float ObjectToPlace::calcInnerRadius() const
{
	return newOuterRadius / srcBuffers[srcBuferIndex].outerRadius * srcBuffers[srcBuferIndex].innerRadius;
}

//--------------------------------------------------------------------------------------------

void calcPosForNewObject(int& bestX_, int& bestY_, float innerRadiusOfNewObject, float density)
{
	// Найти центр описанного бокса для всех текущих позиций объектов
	int xMin = INT_MAX;
	int yMin = INT_MAX;
	int xMax = INT_MIN;
	int yMax = INT_MIN;
	for (const auto& obj : objects) {
		if (obj.x < xMin) xMin = obj.x;
		if (obj.y < yMin) yMin = obj.y;
		if (obj.x > xMax) xMax = obj.x;
		if (obj.y > yMax) yMax = obj.y;
	}
	int xCenter = (xMin + xMax) / 2;
	int yCenter = (yMin + yMax) / 2;

	// Выберем интервалы, в которых будем перебирать точки так, чтобы объект формировался примерно по центру рабочей области
	int x1 = 0;
	int x2 = Scene2DSize / 2;
	if (xCenter < Scene2DSize / 2) {
		x1 = Scene2DSize / 2;
		x2 = Scene2DSize;
	}
	int y1 = 0;
	int y2 = Scene2DSize / 2;
	if (yCenter < Scene2DSize / 2) {
		y1 = Scene2DSize / 2;
		y2 = Scene2DSize;
	}

	// Перебор точек и выбор лучшей для нового объекта
	int bestX = 0;
	int bestY = 0;
	float bestSum = FLT_MAX;

	for (int y = y1; y < y2; ++y) {
		for (int x = x1; x < x2; ++x) {
			// Если точка слишком близко хотя бы к одному объекту, то она не подходит
			for (const auto& obj: objects) {
				float dist = calcDist((float)x, (float)y, (float)obj.x, (float)obj.y);
				if (dist < density*(obj.calcInnerRadius() + innerRadiusOfNewObject)) {
					goto m1;
				}
			}
			// Если точка слишком далеко ото всех объектов, то она не подходит
			int i;
			for (i = 0; i < (int)objects.size(); ++i) {
				const ObjectToPlace& obj = objects[i];
				float dist = calcDist((float)x, (float)y, (float)obj.x, (float)obj.y);
				if (dist < obj.calcInnerRadius() + innerRadiusOfNewObject) {
					break;
				}
			}
			if (i == objects.size()) {
				continue;
			}
			// Из остальных точек выбираем с минимальной суммой расстояний до центров объектов
			float sum;
			sum = 0;
			for (const auto& obj : objects) {
				sum += calcDist((float)x, (float)y, (float)obj.x, (float)obj.y);                        // !!! Попробовать минимизировать сумму квадратов разностей
			}
			if (sum < bestSum) {
				bestSum = sum;
				bestX = x;
				bestY = y;
			}
m1:;
		}
	}
	bestX_ = bestX;
	bestY_ = bestY;
}

//--------------------------------------------------------------------------------------------

void renderLastObjectToDstBufer(int dstBuferIndex)
{
	const ObjectToPlace& obj = objects.back();
	const Buffer& srcBuffer = srcBuffers[obj.srcBuferIndex];
	Buffer& dstBuffer = dstBuffers[dstBuferIndex];
	                                                                    // !!! Добавить произовльный угол вращения и другие параметры
	// Расчитать описаный бокс в dstBuffer, куда будем рендерить объект
	int x1 = std::max(obj.x - (int)obj.newOuterRadius - 10, 0);
	int y1 = std::max(obj.y - (int)obj.newOuterRadius - 10, 0);
	int x2 = std::min(obj.x + (int)obj.newOuterRadius + 10, Scene2DSize-1);
	int y2 = std::min(obj.y + (int)obj.newOuterRadius + 10, Scene2DSize-1);

	float scale = srcBuffer.outerRadius / obj.newOuterRadius;
	for (int y = y1; y <= y2; ++y) {
		for (int x = x1; x <= x2; ++x) {
			int srcX = (int)((x - obj.x) * scale + srcBuffer.centerX);
			int srcY = (int)((y - obj.y) * scale + srcBuffer.centerY);
			float srcVal = 0;
			if (srcX >= 0 && srcX < Scene2DSize && srcY >= 0 && srcY < Scene2DSize) {
				srcVal = srcBuffer.cells[srcX][srcY];
			}
			float dstVal = dstBuffer.cells[x][y];
			dstBuffer.cells[x][y] = 1 - (1 - srcVal) * (1 - dstVal);
		}
	}
}

//--------------------------------------------------------------------------------------------

void test6Generate2dCloud()
{
	srand(5890);
	// Отрендерить круг в нулевой Src буффер
	const float radius = 50;
	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			float dx = x - Scene2DSize / 2.f;
			float dy = y - Scene2DSize / 2.f;
			float dist = sqrt(dx * dx + dy * dy);
			if (dist < radius) {
				float ratio = dist / radius;
				srcBuffers[0].cells[x][y] = (cos(ratio * PI) + 1) * 0.5f * 0.03f;                                // const !!!
			}
		}
	}
	srcBuffers[0].fillParameters();

	// Раскопировать по остальным Src буферам
	for (int i = 1; i < BuffersNum; ++i) {
		srcBuffers[i] = srcBuffers[0];
	}

	const int FractalStepNum = 3;                                                                                // const !!!   
	for (int fractalStep = 0; fractalStep < FractalStepNum; ++fractalStep) {                                                  
		// Расчитаем параметры и отрендерим большие объекта из маленьких
		for (int bufIndex = 0; bufIndex < BuffersNum; ++bufIndex) {
			objects.clear();
			dstBuffers[bufIndex].clear();
			const int SmallObjectInBig = 20;                                                                     // const !!!
			for (int i = 0; i < SmallObjectInBig; ++i) {
				int srcBuferIndex = rand(0, BuffersNum - 1);
				float newOuterRadius = randf(15.f, 50.f);                                                        // const !!!
				float innerRadiusOfNewObject = newOuterRadius / srcBuffers[srcBuferIndex].outerRadius * srcBuffers[srcBuferIndex].innerRadius;
				if (i == 0) {
					objects.emplace_back(ObjectToPlace(Scene2DSize / 2, Scene2DSize / 2, srcBuferIndex, newOuterRadius));
				}
				else {
					int newX = 0;
					int newY = 0;
					float density = fractalStep < FractalStepNum - 1 ? 0.3f : 0.5f;                                  // const !!!
					calcPosForNewObject(newX, newY, innerRadiusOfNewObject, density); 
					objects.emplace_back(ObjectToPlace(newX, newY, srcBuferIndex, newOuterRadius));
				}
				renderLastObjectToDstBufer(bufIndex);
			}

//if (fractalStep == FractalStepNum - 1 && bufIndex == 0) {
//	for (const auto& obj : objects) {
//		printf("%d %d, %f\n", obj.x, obj.y, obj.calcInnerRadius());
//		dstBuffers[bufIndex].cells[obj.x][obj.y] = 1.f;
//	}
//}

			// Рассчитать параметры сгенерированного изображения
			dstBuffers[bufIndex].fillParameters();
			// Сохранить на диск сгенерированное изображение
			char number[4];
			number[0] = bufIndex / 100 + '0';
			number[1] = (bufIndex % 100) / 10 + '0';
			number[2] = (bufIndex % 10) + '0';
			number[3] = 0;
			std::string fname = std::string("Cloud//cloud") + (const char*)number + ".bmp";
			saveDstBufToBmp(fname, bufIndex);
		}
		// Скопируем большие буфера в маленькие
		for (int i = 0; i < BuffersNum; ++i) {
			srcBuffers[i] = dstBuffers[i];
		}
	}
}

//--------------------------------------------------------------------------------------------
// Ренедер
//--------------------------------------------------------------------------------------------

const float FarAway = 100000.f;
const int ScreenSize = 300; // Размер экрана в пикселах
const int SceneSize = 200;  // Размер сцены в единичных кубах
const float cameraZinit = -200; // Позиция камеры по z в системе координат сетки
const float MaxLightBright = 1000; // Максимальная яркость источника света
const double ScatterCoeff = 0.002; // Коэффициент рассеивания тумана  0.00002;
const int SceneDrawNum = 500; // Сколько раз рендерим сцену

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
		if (randf(0.f, 1.f) < scatterProb) {
			scatterProb = 0;                                                  
			dirX = randf(-1.f, 1.f);                   // !!! Рандомный угол выбирать в поляной системе координат, иначе плотность вероятности по телесному углу неравномерна!
			dirY = randf(-1.f, 1.f);
			dirZ = randf(-1.f, 1.f);
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

void test4Render3dScene()
{
	lights.push_back(LIGHT_BOX(MaxLightBright, 10, 70, 0, 30, 150, 200));
	lights.push_back(LIGHT_BOX(MaxLightBright, 180, 170, 0, 200, 200, 200));

	for (int z = 30; z < 170; ++z) {
		for (int y= 130; y < 200; ++y) {
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

//--------------------------------------------------------------------------------------------

void test5()
{
	Scale = 1.5f;

	float x1 = 10;
	float x2 = 11;
	float y1 = 5;
	float y2 = 6;

	Cell2D cell;
	cell.speedX = 0.2f;
	cell.speedY = 0.2f;
	float x = x1;

	// Сначала расширяем
	float xCenter = x1 + 0.5f;
	float yCenter = y1 + 0.5f;
	x1 = (x1 - xCenter) * Scale + xCenter;
	y1 = (y1 - yCenter) * Scale + yCenter;
	x2 = (x2 - xCenter) * Scale + xCenter;
	y2 = (y2 - yCenter) * Scale + yCenter;

	// Корретируем новую позицию, чтобы из-за расширения не зацепить клетки позади по ходу движения
	if (fabs(cell.speedX) > 0.001f || fabs(cell.speedY) > 0.001f) {
		float delta = (float)x - x1; // На сколько увеличенная клетка вылазит за пределы старой клетки (с одной стороны)
		float comX = 0;
		float comY = 0;
		if (fabs(cell.speedX) < fabs(cell.speedY)) {
			float k = fabs(cell.speedX / cell.speedY);
			comY = cell.speedY < 0 ? -delta : delta;
			comX = cell.speedX < 0 ? -delta * k : delta * k;
		}
		else {
			float k = fabs(cell.speedY / cell.speedX);
			comX = cell.speedX < 0 ? -delta : delta;
			comY = cell.speedY < 0 ? -delta * k : delta * k;
		}
		x1 += comX;
		x2 += comX;
		y1 += comY;
		y2 += comY;
	}

	// Сдвиг клиенти по вектору движения
	x1 += cell.speedX;
	y1 += cell.speedY;
	x2 += cell.speedX;
	y2 += cell.speedY;

}

//--------------------------------------------------------------------------------------------


int main()
{
	//test4Render3dScene();
	//test2Render2dAnimation();
	test6Generate2dCloud();
}
