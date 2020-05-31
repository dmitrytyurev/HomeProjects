#include "pch.h"
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <algorithm>
#include "bmp.h"
#include "utils.h"
#include "functional"
#include <time.h>

//--------------------------------------------------------------------------------------------
void generate3dCloud(int randSeed, bool isHardBrush, int size);
void load3dCloud(const std::string& fname, std::vector<float>& dst, int size);

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

void saveToBmp(const std::string& fileName, int sizeX, int sizeY, std::function<uint8_t (int x, int y)> getPixel)
{
	uint8_t* bmpData = new uint8_t[sizeX * sizeY * 3];
	for (int y = 0; y < sizeY; ++y)
	{
		for (int x = 0; x < sizeX; ++x)
		{
			uint8_t bright = getPixel(x, y);
			int pixelOffs = (y * sizeX + x) * 3;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
		}
	}

	save_bmp24(fileName.c_str(), sizeX, sizeY, (const char *)bmpData);
	delete[] bmpData;
}




//--------------------------------------------------------------------------------------------
// Ренедер
//--------------------------------------------------------------------------------------------

const float FarAway = 100000.f;
const int ScreenSize = 200; // Размер экрана в пикселах
const int SceneSize = 200;  // Размер сцены в единичных кубах
const float cameraZinit = -200; // Позиция камеры по z в системе координат сетки
const double ScatterCoeff = 0.4f; // Коэффициент рассеивания тумана  0.00002;
const int SceneDrawNum = 500; // Сколько раз рендерим сцену

struct LIGHT_BOX
{
	LIGHT_BOX(float bright_, float x1_, float y1_, float z1_, float x2_, float y2_, float z2_, bool inner_) : bright(bright_), x1(x1_), y1(y1_), z1(z1_), x2(x2_), y2(y2_), z2(z2_), inner(inner_) {}

	float bright = 0;
	float x1 = 0;
	float x2 = 0;
	float y1 = 0;
	float y2 = 0;
	float z1 = 0;
	float z2 = 0;
	bool inner = false;
};

struct Cell
{
	float smokeDens = 0;
	float normalX = 1;
	float normalY = 0;
	float normalZ = 0;
	float surfaceCoeff = 0;  // 0 - анизатропная среда, 1 - поверхность с резким градиентом
};


double screen[ScreenSize][ScreenSize];
std::vector<LIGHT_BOX> lights;
Cell scene[SceneSize][SceneSize][SceneSize];
float sceneBBoxX1;
float sceneBBoxX2;
float sceneBBoxY1;
float sceneBBoxY2;
float sceneBBoxZ1;
float sceneBBoxZ2;

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

void calcNormalsAndSurfInterp()
{
	const int radius = 5;
	for (int z = radius; z < SceneSize-radius; ++z)	{
		for (int y = radius; y < SceneSize - radius; ++y) {
			for (int x = radius; x < SceneSize - radius; ++x) {
				float dx = 0;
				float dy = 0;
				float dz = 0;
				float dist = 0;

				for (int z2 = z - radius; z2 <= z + radius; ++z2) {
					for (int y2 = y - radius; y2 <= y + radius; ++y2) {
						for (int x2 = x - radius; x2 <= x + radius; ++x2) {
							if (x2 < x + radius) {
								float curDx = scene[x2 + 1][y2][z2].smokeDens - scene[x2][y2][z2].smokeDens;
								dx -= curDx;
								dist += curDx * curDx;
							}
							if (y2 < y + radius) {
								float curDy = scene[x2][y2 + 1][z2].smokeDens - scene[x2][y2][z2].smokeDens;
								dy -= curDy;
								dist += curDy * curDy;
							}
							if (z2 < z + radius) {
								float curDz = scene[x2][y2][z2 + 1].smokeDens - scene[x2][y2][z2].smokeDens;
								dz -= curDz;
								dist += curDz * curDz;
							}
						}
					}
				}
				bool succes = false;
				normalize(dx, dy, dz, succes);
				float surfaceCoeff = 0;
//if (x == 50 && y == 140 && z == 55) {
//printf("%f %f %f\n", dx, dy, dz);
//for (int z2 = z - radius; z2 <= z + radius; ++z2) {
//	for (int y2 = y - radius; y2 <= y + radius; ++y2) {
//		for (int x2 = x - radius; x2 <= x + radius; ++x2) {
//			if (x2 < x + radius) {
//				printf("x: %f %f\n", scene[x2 + 1][y2][z2].smokeDens, scene[x2][y2][z2].smokeDens);
//			}
//			if (y2 < y + radius) {
//				printf("y: %f %f\n", scene[x2][y2 + 1][z2].smokeDens, scene[x2][y2][z2].smokeDens);
//			}
//			if (z2 < z + radius) {
//				printf("z: %f %f\n", scene[x2][y2][z2 + 1].smokeDens, scene[x2][y2][z2].smokeDens);
//			}
//		}
//	}
//}
//}

				if (!succes) {
					dx = 1.f;
					dy = 0.f;
					dz = 0.f;
				}
				else {
					surfaceCoeff = dist * 0.5f;                                           //  !!! const
					surfaceCoeff = std::min(std::max(surfaceCoeff, 0.f), 1.f);
					if (surfaceCoeff > 0.4f)                                              //  !!! const
						surfaceCoeff = 1.f;
					else
						surfaceCoeff = 0.f;
				}
				scene[x][y][z].normalX = dx;
				scene[x][y][z].normalY = dy;
				scene[x][y][z].normalZ = dz;
				scene[x][y][z].surfaceCoeff = surfaceCoeff;
//scene[x][y][z].surfaceCoeff = 0;                                       // Отключить cosine (чтобы сделать облако из дыма)
			}
		}
	}
}

//--------------------------------------------------------------------------------------------
// Определить цвет пиксела и записать его в буфер screen
//--------------------------------------------------------------------------------------------



void renderPixel(int xi, int yi, float x, float y, float z, float dirX, float dirY, float dirZ)
{
	double lightDropAbs = 0;
	double lightDropMul = 1;

	while(true) {
		if (x < sceneBBoxX1 || x > sceneBBoxX2 || y < sceneBBoxY1 || y > sceneBBoxY2 || z < sceneBBoxZ1 || z > sceneBBoxZ2) {
			return;
		}
		for (const auto& light : lights) {
			if (x > light.x1 && x < light.x2 && y > light.y1 && y < light.y2 && z > light.z1 && z < light.z2) {
				if (!light.inner) {
					screen[xi][yi] += std::max(light.bright - lightDropAbs, 0.) * lightDropMul;
				}
				else {
					screen[xi][yi] += std::max(light.bright - lightDropAbs, 0.);
				}
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
		float dist = calcDist(x, y, z, newX, newY, newZ);

		Cell cell;
		if (cubeX >= 0 && cubeX < SceneSize && cubeY >= 0 && cubeY < SceneSize && cubeZ >= 0 && cubeZ < SceneSize) {
			cell = scene[cubeX][cubeY][cubeZ];
		}

		double curScatterProb = cell.smokeDens * dist * ScatterCoeff;
		bool needScatter = false;
		needScatter = randf(0.f, 1.f) < curScatterProb;

		if (needScatter) {
//screen[xi][yi] += cell.surfaceCoeff * 255;
//return;

			dirX = randf(-1.f, 1.f);                // !!! Рандомный угол выбирать в поляной системе координат, иначе плотность вероятности по телесному углу неравномерна!
			dirY = randf(-1.f, 1.f);
			dirZ = randf(-1.f, 1.f);
			bool succes = false;
			normalize(dirX, dirY, dirZ, succes);
			if (!succes) {
				dirX = 1.f;
				dirY = 0;
				dirZ = 0;
			}
			float mul = std::max(cell.normalX * dirX + cell.normalY * dirY + cell.normalZ * dirZ, 0.f);
			const float qw = 0.636f;                                                                        // !!! const
			mul = qw - (qw - mul) * cell.surfaceCoeff;  // surfaceCoeff интерполирует между mul и 1 (убирает влияение mul тем сильнее, чем более анизотропная среда)
			lightDropMul *= mul;
		}
		else {
			x = newX;
			y = newY;
			z = newZ;
		}

		//float add = cell.smokeDens * dist * 4.f;                               // !!! const
		//add *= (1.f - cell.surfaceCoeff);                   
		//lightDropAbs += add;
		//if (lightDropAbs > MaxLightBright) {
		//	return;
		//}
	}
}

//--------------------------------------------------------------------------------------------
// Рендерить сцену в буфер screen
//--------------------------------------------------------------------------------------------

void renderScene(int x1, int y1, int x2, int y2, int frame, int screenSize)
{
	float cameraX = SceneSize / 2.f;
	float cameraY = SceneSize / 2.f;
	float cameraZ = cameraZinit;

	double ratio = (double)SceneSize / (double)ScreenSize;
	for (int yi = y1; yi < y2; ++yi)
	{
		for (int xi = x1; xi < x2; ++xi)
		{
			if (frame > 200 && xi > 0 && xi < screenSize-1 && yi > 0 && yi < screenSize - 1) {
				if (screen[xi][yi] == 0 && screen[xi + 1][yi] == 0 && screen[xi - 1][yi] == 0 && screen[xi][yi + 1] == 0 && screen[xi][yi - 1] == 0) {
					continue;
				}
			}

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

void fillSceneBbox()
{
	sceneBBoxX1 = 0;
	sceneBBoxY1 = 0;
	sceneBBoxZ1 = 0;
	sceneBBoxX2 = SceneSize;
	sceneBBoxY2 = SceneSize;
	sceneBBoxZ2 = SceneSize;
	for (const auto& light : lights) {
		if (light.x1 < sceneBBoxX1) {
			sceneBBoxX1 = light.x1;
		}
		if (light.x2 > sceneBBoxX2) {
			sceneBBoxX2 = light.x2;
		}
		if (light.y1 < sceneBBoxY1) {
			sceneBBoxY1 = light.y1;
		}
		if (light.y2 > sceneBBoxY2) {
			sceneBBoxY2 = light.y2;
		}
		if (light.z1 < sceneBBoxZ1) {
			sceneBBoxZ1 = light.z1;
		}
		if (light.z2 > sceneBBoxZ2) {
			sceneBBoxZ2 = light.z2;
		}
	}
}

//--------------------------------------------------------------------------------------------

void loadObjects()
{
	std::vector<float> cloudBuf;

	load3dCloud("Cloud.bin", cloudBuf, SceneSize);
	for (int z = 0; z < SceneSize; ++z) {
		for (int y = 0; y < SceneSize; ++y) {
			for (int x = 0; x < SceneSize; ++x) {
				scene[x][y][z].smokeDens = cloudBuf[z*SceneSize*SceneSize + y * SceneSize + x];
			}
		}
	}

	//load3dCloud("Objects/Smoke.bin", cloudBuf, SceneSize);
	//for (int z = 0; z < SceneSize; ++z) {
	//	for (int y = 0; y < SceneSize; ++y) {
	//		for (int x = 0; x < 65; ++x) {
	//			scene[x][y][z].smokeDens = cloudBuf[z*SceneSize*SceneSize + y * SceneSize + x];
	//		}
	//	}
	//}
	//load3dCloud("Objects/CloudHard.bin", cloudBuf, SceneSize);
	//for (int z = 0; z < SceneSize; ++z) {
	//	for (int y = 0; y < SceneSize; ++y) {
	//		for (int x = 65; x < 100; ++x) {
	//			scene[x][y][z].smokeDens = cloudBuf[z*SceneSize*SceneSize + y * SceneSize + x];
	//		}
	//	}
	//}
	//load3dCloud("Objects/CloudSoft.bin", cloudBuf, SceneSize);
	//for (int z = 0; z < SceneSize; ++z) {
	//	for (int y = 0; y < SceneSize; ++y) {
	//		for (int x = 100; x < 150; ++x) {
	//			scene[x][y][z].smokeDens = cloudBuf[z*SceneSize*SceneSize + y * SceneSize + x];
	//		}
	//	}
	//}
	//load3dCloud("Objects/CloudExtraSoft.bin", cloudBuf, SceneSize);
	//for (int z = 0; z < SceneSize; ++z) {
	//	for (int y = 0; y < SceneSize; ++y) {
	//		for (int x = 150; x < SceneSize; ++x) {
	//			scene[x][y][z].smokeDens = cloudBuf[z*SceneSize*SceneSize + y * SceneSize + x];
	//		}
	//	}
	//}
}

//--------------------------------------------------------------------------------------------

void screenClear()
{
	for (int y = 0; y < ScreenSize; ++y) {
		for (int x = 0; x < ScreenSize; ++x) {
			screen[x][y] = 0.;
		}
	}
}

//--------------------------------------------------------------------------------------------

void render3dScene(int randSeedForLog)
{
	lights.clear();
	screenClear();

	lights.push_back(LIGHT_BOX(6000, 0, 170, 0, 30, 200, 30, false));
	lights.push_back(LIGHT_BOX(12000, 0, -55, 0, 50, -50, 100, false));
	lights.push_back(LIGHT_BOX(8400, 250, 40, -35, 255, 160, 10, false));
//	lights.push_back(LIGHT_BOX(4000, 55, 100, 70,   150, 103, 73, true));

	// Заполнить BBOX сцены по лампочкам
	fillSceneBbox();

	// Загрузить объекты
	loadObjects();

	// Заполняем нормали и коэффициенты поверхности
	calcNormalsAndSurfInterp();

	// Рендерим все кадры сцены
	for (int n=0; n < SceneDrawNum; ++n) {
		printf("Rendernig frame %d of %d\n", n, SceneDrawNum);
		renderScene(0, 0, ScreenSize, ScreenSize, n, ScreenSize);

		//if ((n % 50) == 0 || n == SceneDrawNum-1) {
		//	char number[6];
		//	number[0] = (n % 100000) / 10000 + '0';
		//	number[1] = (n % 10000) / 1000 + '0';
		//	number[2] = (n % 1000) / 100 + '0';
		//	number[3] = (n % 100) / 10 + '0';
		//	number[4] = (n % 10) + '0';
		//	number[5] = 0;
		//	std::string fname = std::string("Scenes/3dScene_Cloud") + (const char*)number + ".bmp";
		//	saveToBmp(fname, ScreenSize, ScreenSize, [n](int x, int y) { return (uint8_t)(std::min(screen[x][y] / (n + 1), 255.)); });
		//}
	}

	char number[6];
	int n = randSeedForLog;
	number[0] = (n % 100000) / 10000 + '0';
	number[1] = (n % 10000) / 1000 + '0';
	number[2] = (n % 1000) / 100 + '0';
	number[3] = (n % 100) / 10 + '0';
	number[4] = (n % 10) + '0';
	number[5] = 0;
	std::string fname = std::string("Scenes/3dScene_Cloud_Rand") + (const char*)number + ".bmp";
	saveToBmp(fname, ScreenSize, ScreenSize, [n](int x, int y) { return (uint8_t)(std::min(screen[x][y] / (SceneDrawNum + 1), 255.)); });
}


//--------------------------------------------------------------------------------------------

void RenderRandom(bool isHardBrush)
{
	for (int i = 1; i < 1000; ++i) {
		generate3dCloud(i, isHardBrush, SceneSize);
		render3dScene(i);
	}
}

//--------------------------------------------------------------------------------------------


int main()
{
	RenderRandom(true);
//	generate3dCloud(4, true, SceneSize);
//	render3dScene(4);

}

