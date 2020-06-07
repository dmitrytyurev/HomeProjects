#include "pch.h"
#include <iostream>
#include <vector>
#include <string>
#include <stdarg.h>
#include <algorithm>
#include "bmp.h"
#include "utils.h"
#include "functional"
#include <time.h>
#include "Nodes.h"

//--------------------------------------------------------------------------------------------
void rasterizeCloud(std::vector<float>& dst, int bufSize, int randSeed, bool isHardBrush, float brushCoeff, float xPos, float yPos, float zPos, float scale, bool generateOrLoad);
void load3dCloud(const std::string& fname, std::vector<float>& dst, int size);

extern float leafScale;
//--------------------------------------------------------------------------------------------
// Ренедер
//--------------------------------------------------------------------------------------------

const float FarAway = 100000.f;
const int ScreenSize = 400; // Размер экрана в пикселах
const int SceneSize = 200;  // Размер сцены в единичных кубах
const float cameraZinit = -500; // Позиция камеры по z в системе координат сетки
const double ScatterCoeff = 0.4f; // Коэффициент рассеивания тумана  0.00002;
const int SceneDrawNum = 500; // Сколько раз рендерим сцену



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

///--------------------------------------------------------------------------------------------

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
	printf("calcNormalsAndSurfInterp...\n");
	const int radius = 5;                                                                      // !!!const
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
	printf("  Done\n");
}

//--------------------------------------------------------------------------------------------
// Определить цвет пиксела и записать его в буфер screen
//--------------------------------------------------------------------------------------------

struct Dir
{
	Dir() = default;
	Dir(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
	bool operator==(const Dir& l) { return l.x == x && l.y==y && l.z==z; }
	float x;
	float y;
	float z;
};

struct RandomRepeatChecker
{
	void check(int xi, int yi, float dirX, float dirY, float dirZ);
	void onStartNewFrame();

	std::vector<Dir> dirs0;
	std::vector<Dir> dirs1;
	std::vector<Dir> dirs2;
	std::vector<Dir> dirs3;
	std::vector<Dir> dirs4;
	std::vector<Dir> dirs5;
	std::vector<Dir> dirs6;
	std::vector<Dir> dirs7;
	std::vector<Dir> dirs8;
	std::vector<Dir> dirs9;
	int gotchasNum = 0;
};

void RandomRepeatChecker::onStartNewFrame()
{
	dirs0.clear();
	dirs1.clear();
	dirs2.clear();
	dirs3.clear();
	dirs4.clear();
	dirs5.clear();
	dirs6.clear();
	dirs7.clear();
	dirs8.clear();
	dirs9.clear();
}

void RandomRepeatChecker::check(int xi, int yi, float dirX, float dirY, float dirZ)
{

	if (xi == 200 && yi == 200) {
		if (std::find(dirs0.begin(), dirs0.end(), Dir(dirX, dirY, dirZ)) != dirs0.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs0.push_back(Dir(dirX, dirY, dirZ));
	}
	if (xi == 200 - 3 && yi == 200 - 20) {
		if (std::find(dirs1.begin(), dirs1.end(), Dir(dirX, dirY, dirZ)) != dirs1.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs1.push_back(Dir(dirX, dirY, dirZ));
	}
	if (xi == 200 - 7 && yi == 200 + 16) {
		if (std::find(dirs2.begin(), dirs2.end(), Dir(dirX, dirY, dirZ)) != dirs2.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs2.push_back(Dir(dirX, dirY, dirZ));
	}
	if (xi == 200 - 4 && yi == 200 - 5) {
		if (std::find(dirs3.begin(), dirs3.end(), Dir(dirX, dirY, dirZ)) != dirs3.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs3.push_back(Dir(dirX, dirY, dirZ));
	}
	if (xi == 200 + 9 && yi == 200 - 8) {
		if (std::find(dirs4.begin(), dirs4.end(), Dir(dirX, dirY, dirZ)) != dirs4.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs4.push_back(Dir(dirX, dirY, dirZ));
	}
	if (xi == 200 + 7 && yi == 200 - 12) {
		if (std::find(dirs5.begin(), dirs5.end(), Dir(dirX, dirY, dirZ)) != dirs5.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs5.push_back(Dir(dirX, dirY, dirZ));
	}
	if (xi == 200 + 5 && yi == 200 + 6) {
		if (std::find(dirs6.begin(), dirs6.end(), Dir(dirX, dirY, dirZ)) != dirs6.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs6.push_back(Dir(dirX, dirY, dirZ));
	}
	if (xi == 200 + 3 && yi == 200 + 2) {
		if (std::find(dirs7.begin(), dirs7.end(), Dir(dirX, dirY, dirZ)) != dirs7.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs7.push_back(Dir(dirX, dirY, dirZ));
	}
	if (xi == 200 - 15 && yi == 200 + 1) {
		if (std::find(dirs8.begin(), dirs8.end(), Dir(dirX, dirY, dirZ)) != dirs8.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs8.push_back(Dir(dirX, dirY, dirZ));
	}
	if (xi == 200 - 2 && yi == 200 - 4) {
		if (std::find(dirs9.begin(), dirs9.end(), Dir(dirX, dirY, dirZ)) != dirs9.end()) {
			++gotchasNum;
			log("gotcha! " + std::to_string(gotchasNum) + "\n");
		}
		dirs9.push_back(Dir(dirX, dirY, dirZ));
	}
}

RandomRepeatChecker randomRepeatChecker;

void renderPixel(int xi, int yi, float x, float y, float z, float dirX, float dirY, float dirZ, bool draftRender)
{
	double lightDropAbs = 0;
	double lightDropMul = 1;
	bool firstScatter = true;

	while(true) {
		if (x < sceneBBoxX1 || x > sceneBBoxX2 || y < sceneBBoxY1 || y > sceneBBoxY2 || z < sceneBBoxZ1 || z > sceneBBoxZ2) {
			return;
		}
		if (!firstScatter || draftRender) {                             // Влёт в лампочку проверяем только после первого рассеивания, чтобы сами лампочки не отображались
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
			if (draftRender) {
				bool succ = false;
				normalize(dirX, dirY, dirZ, succ);
				float cosin = cell.normalX*(-dirX) + cell.normalY*(-dirY) + cell.normalZ*(-dirZ);
				screen[xi][yi] += fabs(cosin * 255);
				return;
			}

			dirX = randf(-1.f, 1.f);                // !!! Рандомный угол выбирать в поляной системе координат, иначе плотность вероятности по телесному углу неравномерна!
			dirY = randf(-1.f, 1.f);
			dirZ = randf(-1.f, 1.f);
			if (firstScatter) {
				firstScatter = false;
				randomRepeatChecker.check(xi, yi, dirX, dirY, dirZ);
			}
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

void renderSubFrame(int x1, int y1, int x2, int y2, int frame, int screenSize, float cameraAngle, bool draftRender)
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

			float xTurned = 0;
			float yTurned = y;
			float zTurned = 0;
			float dirXTurned = 0;
			float dirYTurned = dirY;
			float dirZTurned = 0;
			turnPointCenter(x, z, cameraAngle, 100, 100, xTurned, zTurned);
			turnPoint(dirX, dirZ, cameraAngle, dirXTurned, dirZTurned);
			renderPixel(xi, yi, xTurned, yTurned, zTurned, dirXTurned, dirYTurned, dirZTurned, draftRender);
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


void copyToScene(std::vector<float>& src)
{
	for (int z = 0; z < SceneSize; ++z) {
		for (int y = 0; y < SceneSize; ++y) {
			for (int x = 0; x < SceneSize; ++x) {
				scene[x][y][z].smokeDens = src[z*SceneSize*SceneSize + y * SceneSize + x];
			}
		}
	}
}

//--------------------------------------------------------------------------------------------

void addToScene(std::vector<float>& src, float alpha=1.f, bool flipZ=false)
{
	for (int z = 0; z < SceneSize; ++z) {
		for (int y = 0; y < SceneSize; ++y) {
			for (int x = 0; x < SceneSize; ++x) {
				int curZ = !flipZ ? z : SceneSize - 1 - z;
				float srcVal = src[curZ*SceneSize*SceneSize + y * SceneSize + x] * alpha;
				float dstVal = scene[x][y][z].smokeDens;
				scene[x][y][z].smokeDens = 1.f - (1.f - srcVal) * (1.f - dstVal);
			}
		}
	}
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

void renderFrame(const std::string& fileNamePrefix, int frameN, float cameraAngle, bool draftRender)
{
	lights.clear();
	screenClear();
	randomRepeatChecker.onStartNewFrame();

	lights.push_back(LIGHT_BOX(4500, 0, 170, 0, 30, 200, 30, false));
	lights.push_back(LIGHT_BOX(12000, 0, -55, 0, 50, -50, 100, false));
	lights.push_back(LIGHT_BOX(8400, 250, 0, -35, 255, 120, 10, false)); 
	lights.push_back(LIGHT_BOX(16000, 250, 133, 190, 255, 173, 235, false));
	lights.push_back(LIGHT_BOX(14000, -20, 50, 190, -15, 80, 235, false));

//	lights.push_back(LIGHT_BOX(4000, 55, 100, 70,   150, 103, 73, true));

	// Заполнить BBOX сцены по лампочкам
	fillSceneBbox();

	// Рендерим все кадры сцены
	for (int n=0; n < SceneDrawNum; ++n) {
		srand(n);
		printf("Rendernig subframe %d/%d of frame %d\n", n, SceneDrawNum, frameN);
		renderSubFrame(0, 0, ScreenSize, ScreenSize, n, ScreenSize, cameraAngle, draftRender);

		if (!draftRender && (n % 50) == 0) {
			std::string fname = std::string("Scenes/Frame") + digit5intFormat(frameN) + "_" + digit5intFormat(n) + ".bmp";
			saveToBmp(fname, ScreenSize, ScreenSize, [n](int x, int y) { return (uint8_t)(std::min(screen[x][y] / (n + 1), 255.)); });
		}
	}

	std::string fname = fileNamePrefix + digit5intFormat(frameN) + ".bmp";
	saveToBmp(fname, ScreenSize, ScreenSize, [](int x, int y) { return (uint8_t)(std::min(screen[x][y] / (SceneDrawNum + 1), 255.)); });
}

//--------------------------------------------------------------------------------------------

void RenderRandom()
{
	std::vector<float> rasterizeBuf;

	for (int i = 0; i < 100; ++i) {
		rasterizeCloud(rasterizeBuf, SceneSize, i, true, 0.04f, SceneSize / 2, SceneSize / 2, SceneSize / 2, 1.f, true);
		copyToScene(rasterizeBuf);
		calcNormalsAndSurfInterp();
		renderFrame("Scenes/3dScene_Cloud_Rand", i, 0.f, false);
	}
}

//--------------------------------------------------------------------------------------------

void rasterizeScene()
{
	std::vector<float> rasterizeBuf;
	rasterizeCloud(rasterizeBuf, SceneSize, 14, true, 0.04f, 265 / 2, 112 / 2, 100, 0.675f, false);
	copyToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 12, true, 0.04f, 102 / 2, 252 / 2, 100, 0.54f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 156, 198 / 2, 100, 0.468f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 10, true, 0.04f, 201 / 2, 215 / 2, 60, 0.75f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 72, 63, 100, 0.75f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 4, true, 0.04f, 103, 175, 100, 0.54f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 17, false, 0.04f, 140, 147, 115, 0.6f*1.2f, false);   // Облако
	addToScene(rasterizeBuf, 0.09f);
	rasterizeCloud(rasterizeBuf, SceneSize, 0, false, 0.04f, 120, 147, 90, 0.6f, false);   // Облако
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 17, false, 0.04f, 85, 165, 75, 0.6f*1.15f, false);   // Облако
	addToScene(rasterizeBuf, 0.2f);
	rasterizeCloud(rasterizeBuf, SceneSize, 0, false, 0.04f, 100, 60, 140, 0.6f, false);   // Облако
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 0, false, 0.04f, 85, 100, 50, 0.6f*1.1f, false);   // Облако лево центр
	addToScene(rasterizeBuf, 0.5f);
	   
	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 103, 180, 80, 0.3f, false);  // Маленькие у подножия
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 90, 185, 130, 0.3f, false);  // Маленькие у подножия
	addToScene(rasterizeBuf);
	// Обратная сторона
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 100, 118, 130, 0.8f, false);
	addToScene(rasterizeBuf);

	// Заполняем нормали и коэффициенты поверхности
	calcNormalsAndSurfInterp();
}

//--------------------------------------------------------------------------------------------
const int framesInTurn = 30;
const bool draft = false;

void RenderRotate(int fromFrame, int toFrame)
{
	rasterizeScene();
	for (int i = fromFrame; i <= toFrame; ++i) {
		renderFrame("Scenes/3dScene_Cloud_Rotate", i, PI / (framesInTurn-1) * i, draft);
	}
}

//--------------------------------------------------------------------------------------------

void RenderAnimate(int fromFrame, int toFrame)
{
	for (int i = fromFrame; i <= toFrame; ++i) {
		leafScale = 1.f + i * 0.5f;
		rasterizeScene();
		renderFrame("Scenes/3dScene_Cloud_Rotate", i, 0, draft);
	}
}
//--------------------------------------------------------------------------------------------

int main(int argc, char *argv[], char *envp[])
{
	int fromFrame = 0;
	int toFrame = framesInTurn-1;

	if (argc == 3) {
		fromFrame = atoi(argv[1]);
		toFrame = atoi(argv[2]);
	}

	//RenderRotate(fromFrame, toFrame);
	RenderAnimate(fromFrame, toFrame);
}

