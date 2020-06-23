#include "pch.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <string>
#include <stdarg.h>
#include <algorithm>
#include <functional>
#include <fstream>
#include <set>
#include <time.h>

#include "bmp.h"
#include "utils.h"
#include "Nodes.h"

//--------------------------------------------------------------------------------------------
void rasterizeCloud(std::vector<float>& dst, int bufSize, int randSeed, bool isHardBrush, float brushCoeff, float xPos, float yPos, float zPos, float scale, bool generateOrLoad);
void load3dCloud(const std::string& fname, std::vector<float>& dst, int size);

extern float leafScale;
//--------------------------------------------------------------------------------------------
// Ренедер
//--------------------------------------------------------------------------------------------

const std::string FileNamePrefix = "Scene";
const std::string inPath = "../Source/";
const std::string outPath = "../ToSend/";
const std::string intermediatePath = "../Tmp/";
const float lightBright = 7000.f;
const float FarAway = 100000.f;
const int ScreenSize = 400; // Размер экрана в пикселах
const int SceneSize = 200;  // Размер сцены в единичных кубах
const float cameraZinit = -500; // Позиция камеры по z в системе координат сетки
const double ScatterCoeff = 0.4f; // Коэффициент рассеивания тумана

int SubframesInOneFrame = 500; // Сколько раз рендерим сцену
bool draft = false;
bool useCosineMul = true;
float zoom = 1.f;       // Увеличение камеры
float scenesInterp = 0; // 0 - сцена с вулканом, 1 - сцена с облаками
float cloudsFlow = 0;   // Лёгкие облака бегут над полем из облаков

float draftLightning = 0;

//--------------------------------------------------------------------------------------------
struct Circle
{
	int xCenter = 0;
	int yCenter = 0;
	int diametr = 0;
};
std::vector<Circle> smokeOfcircles = { { 177, 333, 54 }, { 171, 292, 70 }, {185, 248, 86 }, {215, 191, 100 }, {225, 116, 133 }, { 215, 14, 185 } };
//--------------------------------------------------------------------------------------------


void saveToBmp(const std::string& fileName, int sizeX, int sizeY, std::function<uint8_t (int x, int y)> getPixel)
{
	uint8_t* bmpData = new uint8_t[sizeX * sizeY * 3];
	for (int y = 0; y < sizeY; ++y)	{
		for (int x = 0; x < sizeX; ++x)	{
			uint8_t bright = getPixel(x, y);
			int pixelOffs = (y * sizeX + x) * 3;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
		}
	}

	save_bmp24(fileName.c_str(), sizeX, sizeY, (const unsigned char *)bmpData);
	delete[] bmpData;
}

//--------------------------------------------------------------------------------------------

void saveToFlt(const std::string& fileName, int sizeX, int sizeY, int subframeLast, std::function<double (int x, int y)> getPixel)
{
	FILE* f = fopen(fileName.c_str(), "wb");
	if (!f) {
		return;
	}
	std::vector<double> data;
	data.resize(sizeX * sizeY);;
	for (int y = 0; y < sizeY; ++y) {
		for (int x = 0; x < sizeX; ++x) {
			data[y*sizeX + x] = getPixel(x, y);
		}
	}

	fwrite(&subframeLast, sizeof(int), 1, f);
	fwrite(&data[0], data.size() * sizeof(double), 1, f);
	fclose(f);
}

//--------------------------------------------------------------------------------------------

void loadFromFlt(const std::string& fileName, int sizeX, int sizeY, int subframeFirst, std::function<double&(int x, int y)> getPixel)
{
	FILE* f = fopen(fileName.c_str(), "rb");
	if (!f) {
		exit_msg("Can't open file %s", fileName.c_str());
	}
	std::vector<double> data;
	data.resize(sizeX * sizeY);;

	int subframeLast = 0;
	fread(&subframeLast, sizeof(int), 1, f);
	if (subframeFirst != subframeLast + 1) {
		exit_msg("subframeFirst: %d, subframeLast: %d", subframeFirst, subframeLast);
	}

	fread(&data[0], data.size() * sizeof(double), 1, f);
	for (int y = 0; y < sizeY; ++y) {
		for (int x = 0; x < sizeX; ++x) {
			getPixel(x, y) = data[y*sizeX + x];
		}
	}
	fclose(f);
}

//--------------------------------------------------------------------------------------------

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
	printf("calcNormalsAndSurfInterp... ");
	const int radius = draft ? 2: 5;                                                                      // !!!const
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

void renderPixel(int xi, int yi, float x, float y, float z, float dirX, float dirY, float dirZ)
{
	double lightDropAbs = 0;
	double lightDropMul = 1;
	bool firstScatter = true;

	while(true) {
		if (x < sceneBBoxX1 || x > sceneBBoxX2 || y < sceneBBoxY1 || y > sceneBBoxY2 || z < sceneBBoxZ1 || z > sceneBBoxZ2) {
			return;
		}
		if (!firstScatter /*|| draft*/) {                             // Влёт в лампочку проверяем только после первого рассеивания, чтобы сами лампочки не отображались
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
			if (draft) {
				bool succ = false;
				normalize(dirX, dirY, dirZ, succ);
				float cosin = cell.normalX*(-dirX) + cell.normalY*(-dirY) + cell.normalZ*(-dirZ);
				float br = fabs(cosin * 255);
				br = (255.f - br) * draftLightning + br;
				screen[xi][yi] += br;
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
			if (useCosineMul) {
				lightDropMul *= mul;
			}
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

void renderSubFrame(int x1, int y1, int x2, int y2, int frame, int screenSize, float cameraAngleA, float cameraAngleB, float cameraYOffs)
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
			float y = (float)((((double)yi) + 0.5f) * ratio) + cameraYOffs;
			x = (float)((x - SceneSize *0.5) / zoom + SceneSize *0.5);
			y = (float)((y - SceneSize *0.5) / zoom + SceneSize *0.5);
			float z = 0;
			float dirX = x - cameraX;
			float dirY = y - cameraY;
			float dirZ = z - cameraZ;

			float xTurned = x;
			float yTurned = 0;
			float zTurned = 0;
			float dirXTurned = dirX;
			float dirYTurned = 0;
			float dirZTurned = 0;

			// Наклон камеры
			turnPointCenter(z, y, cameraAngleB, 100, 80, zTurned, yTurned);
			turnPoint(dirZ, dirY, cameraAngleB, dirZTurned, dirYTurned);

			float xTurned2 = 0;
			float yTurned2 = yTurned;
			float zTurned2 = 0;
			float dirXTurned2 = 0;
			float dirYTurned2 = dirYTurned;
			float dirZTurned2 = 0;

			// Вращение камеры в горизонтальной плоскости
			turnPointCenter(xTurned, zTurned, cameraAngleA, 100, 100, xTurned2, zTurned2);
			turnPoint(dirXTurned, dirZTurned, cameraAngleA, dirXTurned2, dirZTurned2);
			// Рендер пиксела
			renderPixel(xi, yi, xTurned2, yTurned2, zTurned2, dirXTurned2, dirYTurned2, dirZTurned2);
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
// Время в секундах, которое ренедрится один субфрейм каждого кадра 
std::vector<int> durationOfSceneSetup = { 67,67,68,67,67,67,68,69,69,67,68,68,69,68,68,69,69,67,67,68,68,68,68,69,69,68,67,67,68,68,68,68,69,70,68,68,67,67,67,73,75,74,72,42,93,93,94,95,93,90,89,90,89,90,89,90,89,90,89,89,89,89};

// Время в десятых долях секунды, которое ренедрится один субфрейм каждого кадра 
// До 200 кадра (оптимизация)
//std::vector<int> durationOfOneSubfarme = { 50,40,40,50,50,50,50,40,40,50,40,40,40,50,50,50,50,40,40,40,40,50,40,40,40,60,70,70,70,70,70,70,60,60,40,40,40,50,50,70,50,50,40,20,40,50,60,60,70,100,120,130,130,120,120,120,120,120,120,120,120,130,120};
// После 200 кадра (нет оптимизации)
std::vector<int> durationOfOneSubfarme = { 27,27,27,28,29,29,29,30,30,29,29,31,31,32,32,33,33,28,28,29,29,30,31,32,31,53,66,67,67,64,65,66,66,67,28,29,29,29,30,30,31,29,18,7,1,32,37,40,45,70,103,119,120,120,119,119,119,119,119,119,120,119,120 };

void estimateFinish(int frameN, int subframeN)
{
	int subframeFirst = -1;
	int subframeLast = -1;
	float estmatedTimeLeft = 0;

	std::ifstream infile("config.txt");
	int val1 = 0;
	int val2 = 0;
	while (infile >> val1 >> val2)
	{
		if (subframeFirst == -1) {
			subframeFirst = val1;
			subframeLast = val2;
		}
		else {
			for (int frame = val1; frame <= val2; ++frame) {
				std::string fname = outPath + FileNamePrefix + digit5intFormat(frame) + ".bmp";
				if (!std::experimental::filesystem::exists(fname)) {
					int index = (frame + 5) / 10;
					index = std::min(index, (int)durationOfOneSubfarme.size() - 1);
					index = std::min(index, (int)durationOfSceneSetup.size() - 1);
					if (frame == frameN) {
						estmatedTimeLeft += (subframeLast - subframeN + 1) * durationOfOneSubfarme[index] * 0.1f;
					}
					else {
						estmatedTimeLeft += (subframeLast - subframeFirst + 1) * durationOfOneSubfarme[index] * 0.1f + durationOfSceneSetup[index];
					}
				}
			}
		}
	}
	int hoursLeft = ((int)estmatedTimeLeft / 3600);
	int minutesLeft = ((int)estmatedTimeLeft / 60) % 60;
	printf("estimated time left: %d hours %d minutes\n", hoursLeft, minutesLeft);
}

//--------------------------------------------------------------------------------------------

void renderFrameImpl(int frameN, float cameraAngleA, float cameraAngleB, float cameraYOffs, int subframeFirst, int subframeLast)
{
	draftLightning = 0.f;
	if (draft) {
		for (auto& light : lights) {
			if (light.inner) {
				draftLightning = std::min(light.bright / lightBright, 1.f);
				break;
			}
		}
	}

	randomRepeatChecker.onStartNewFrame();
	screenClear();
	if (subframeFirst) {
		std::string fname = inPath + FileNamePrefix + digit5intFormat(frameN) + ".flt";
		loadFromFlt(fname, ScreenSize, ScreenSize, subframeFirst, [](int x, int y)->double& { return screen[x][y]; });
	}

	// Рендерим все суб-кадры текущего кадра
	for (int n=subframeFirst; n <= subframeLast; ++n) {
		srand(n);
		printf("Rendering frame %d, subframe %d (up to %d)\n", frameN, n, subframeLast);
		renderSubFrame(0, 0, ScreenSize, ScreenSize, n, ScreenSize, cameraAngleA, cameraAngleB, cameraYOffs);

		estimateFinish(frameN, n);

		if (!draft && (n % 50) == 0) {
			std::string fname = intermediatePath + "Frame" + digit5intFormat(frameN) + "_" + digit5intFormat(n) + ".bmp";
			saveToBmp(fname, ScreenSize, ScreenSize, [n](int x, int y) { return (uint8_t)(std::min(screen[x][y] / (n + 1), 255.)); });
		}
	}
	std::string fname = outPath + FileNamePrefix + digit5intFormat(frameN) + ".flt";
	saveToFlt(fname, ScreenSize, ScreenSize, subframeLast, [](int x, int y) { return screen[x][y]; });

	fname = outPath + FileNamePrefix + digit5intFormat(frameN) + ".bmp";
	saveToBmp(fname, ScreenSize, ScreenSize, [subframeLast](int x, int y) { return (uint8_t)(std::min(screen[x][y] / (subframeLast + 1), 255.)); });
}

//--------------------------------------------------------------------------------------------

void setupSceneVulcano()
{
	useCosineMul = true;
	float yOff = 0; // scenesInterp / 0.5f * SceneSize;

	std::vector<float> rasterizeBuf;
	rasterizeCloud(rasterizeBuf, SceneSize, 14, true, 0.04f, 265 / 2, 112 / 2 + yOff, 100, 0.675f, false);
	copyToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 12, true, 0.04f, 102 / 2, 252 / 2 + yOff, 100, 0.54f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 156, 198 / 2 + yOff, 100, 0.468f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 10, true, 0.04f, 201 / 2, 215 / 2 + yOff, 60, 0.75f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 72, 63 + yOff, 100, 0.75f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 4, true, 0.04f, 103, 175 + yOff, 100, 0.54f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 17, false, 0.04f, 140, 147 + yOff, 115, 0.6f*1.2f, false);   // Облако
	addToScene(rasterizeBuf, 0.09f);
	rasterizeCloud(rasterizeBuf, SceneSize, 0, false, 0.04f, 120, 147 + yOff, 90, 0.6f, false);   // Облако
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 17, false, 0.04f, 85, 165 + yOff, 75, 0.6f*1.15f, false);   // Облако
	addToScene(rasterizeBuf, 0.2f);
	rasterizeCloud(rasterizeBuf, SceneSize, 0, false, 0.04f, 100, 60 + yOff, 140, 0.6f, false);   // Облако
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 0, false, 0.04f, 85, 100 + yOff, 50, 0.6f*1.1f, false);   // Облако лево центр
	addToScene(rasterizeBuf, 0.5f);
	   
	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 103, 180 + yOff, 80, 0.3f, false);  // Маленькие у подножия
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 90, 185 + yOff, 130, 0.3f, false);  // Маленькие у подножия
	addToScene(rasterizeBuf);
	// Обратная сторона
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 100, 118 + yOff, 130, 0.8f, false);
	addToScene(rasterizeBuf);

	// Заполняем нормали и коэффициенты поверхности
	calcNormalsAndSurfInterp();

	lights.clear();
	lights.push_back(LIGHT_BOX(4500, 0, 170 + yOff, 0, 30, 200 + yOff, 30, false));
	lights.push_back(LIGHT_BOX(12000, 0, -55 + yOff, 0, 50, -50 + yOff, 100, false));
	lights.push_back(LIGHT_BOX(8400, 250, 0 + yOff, -35, 255, 120 + yOff, 10, false));
	lights.push_back(LIGHT_BOX(20000, 250, 133 + yOff, 190, 255, 173 + yOff, 235, false));
	lights.push_back(LIGHT_BOX(14000, -20, 50 + yOff, 190, -15, 80 + yOff, 235, false));

	lights.push_back(LIGHT_BOX(0, 90, 90, 90, 110, 110 + yOff, 110, true));   // Внутренний свет (молния) 

	// Заполнить BBOX сцены по лампочкам
	fillSceneBbox();
}

//--------------------------------------------------------------------------------------------

void setupSceneHeaven()
{
	int maxOffs = SceneSize + 20;
	float yOff = (scenesInterp - 0.5f) / 0.5f * maxOffs - maxOffs;
	useCosineMul = false;

	std::vector<float> rasterizeBuf;
	rasterizeCloud(rasterizeBuf, SceneSize, 4, true, 0.04f, 10, 160+ yOff, 10, 0.475f, false);
	copyToScene(rasterizeBuf);

	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 40, 160 + yOff, 10, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 70, 160 + yOff, 10, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 10, true, 0.04f, 100, 160 + yOff, 10, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 11, true, 0.04f, 130, 160 + yOff, 10, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 12, true, 0.04f, 160, 160 + yOff, 10, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 14, true, 0.04f, 190, 190 + yOff, 10, 0.475f, false);
	addToScene(rasterizeBuf);

	rasterizeCloud(rasterizeBuf, SceneSize, 12, true, 0.04f, 10, 160 + yOff, 40, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 11, true, 0.04f, 40, 160 + yOff, 40, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 4, true, 0.04f, 70, 160 + yOff, 40, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 14, true, 0.04f, 100, 160 + yOff, 40, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 130, 160+ yOff, 40, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 160, 160+ yOff, 40, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 10, true, 0.04f, 190, 190 + yOff, 40, 0.475f, false);
	addToScene(rasterizeBuf);

	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 10, 160+ yOff, 70, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 4, true, 0.04f, 40, 160+ yOff, 70, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 12, true, 0.04f, 70, 160+ yOff, 70, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 11, true, 0.04f, 100, 160+ yOff, 70, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 10, true, 0.04f, 130, 160+ yOff, 70, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 14, true, 0.04f, 160, 160+ yOff, 70, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 190, 190 + yOff, 70, 0.475f, false);
	addToScene(rasterizeBuf);

	rasterizeCloud(rasterizeBuf, SceneSize, 14, true, 0.04f, 10, 160+ yOff, 100, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 10, true, 0.04f, 40, 160+ yOff, 100, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 11, true, 0.04f, 70, 160+ yOff, 100, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 100, 160+ yOff, 100, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 130, 160+ yOff, 100, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 12, true, 0.04f, 160, 160+ yOff, 100, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 4, true, 0.04f, 190, 190 + yOff, 100, 0.475f, false);
	addToScene(rasterizeBuf);

	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 10, 160+ yOff, 130, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 10, true, 0.04f, 40, 160+ yOff, 130, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 4, true, 0.04f, 70, 160+ yOff, 130, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 100, 160+ yOff, 130, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 12, true, 0.04f, 130, 160+ yOff, 130, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 14, true, 0.04f, 160, 160+ yOff, 130, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 11, true, 0.04f, 190, 190 + yOff, 130, 0.475f, false);
	addToScene(rasterizeBuf);

	rasterizeCloud(rasterizeBuf, SceneSize, 12, true, 0.04f, 10, 160 + yOff, 160, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 14, true, 0.04f, 40, 160 + yOff, 160, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 11, true, 0.04f, 70, 160 + yOff, 160, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 10, true, 0.04f, 100, 160 + yOff, 160, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 130, 160 + yOff, 160, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 4, true, 0.04f, 160, 160 + yOff, 160, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 190, 190 + yOff, 160, 0.475f, false);
	addToScene(rasterizeBuf);

	rasterizeCloud(rasterizeBuf, SceneSize, 12, true, 0.04f, 10, 160+ yOff, 190, 0.875f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 11, true, 0.04f, 40, 160+ yOff, 190, 0.5f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 4, true, 0.04f, 70, 160+ yOff, 190, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 14, true, 0.04f, 100, 160+ yOff, 190, 0.6f, false);  // 1.
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 5, true, 0.04f, 130, 160+ yOff, 190, 0.475f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 7, true, 0.04f, 160, 160+ yOff, 190, 1.f, false);
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 10, true, 0.04f, 190, 190 + yOff, 190, 0.675f, false);
	addToScene(rasterizeBuf);

	rasterizeCloud(rasterizeBuf, SceneSize, 0, false, 0.04f, 110 - cloudsFlow * 20, 110 + yOff, 165, 0.6f*0.7f*0.5f, false);   // Облако
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 17, false, 0.04f, 50 - cloudsFlow * 25, 110 + yOff, 140, 0.6f*0.7f*0.6f, false);   // Облако
	addToScene(rasterizeBuf);
	rasterizeCloud(rasterizeBuf, SceneSize, 17, false, 0.04f, 150 - cloudsFlow * 60, 110 + yOff, 80, 0.6f*0.9f*0.65f, false);   // Облако
	addToScene(rasterizeBuf);

	// Заполняем нормали и коэффициенты поверхности
	calcNormalsAndSurfInterp();

	lights.clear();
	lights.push_back(LIGHT_BOX(6400*0.5f, 800, -400 + yOff, -100, 805, 0 + yOff, 300, false));
	lights.push_back(LIGHT_BOX(1400*0.6f, 200, -400 + yOff, -100, 800, -395 + yOff, 300, false));

	// Заполнить BBOX сцены по лампочкам
	fillSceneBbox();
}

//--------------------------------------------------------------------------------------------

float getInterp(std::vector<std::pair<float, float>>& v, float time)
{
	for (int i=0; i<(int)v.size(); ++i) {
		if (v[i].first >= time) {
			if (i > 0) {
				float ratio = (time - v[i - 1].first) / (v[i].first - v[i - 1].first);
				return (v[i].second - v[i - 1].second) * ratio + v[i - 1].second;
			}
			else {
				return v[i].second;
			}
		}
	}
	return v.back().second;
}

//--------------------------------------------------------------------------------------------

void setLightning(float bright)
{
	for (auto& light : lights) {
		if (light.inner) {
			light.bright = bright;
		}
	}
}
//--------------------------------------------------------------------------------------------

std::vector<std::pair<float, float>> cameraAlSpeedTrack = { {2.99f, 0.f}, {3.f, -0.142f}, {3.4f, -0.142f}, {3.41f, -0.004f}, {6.41f, -0.004f}, {6.42f, -0.142f}, {6.79f, -0.142f}, {6.8f, 0.f} };
std::vector<std::pair<float, float>> lightningAnimTrack = { {8.8f, 0.f}, {8.81f, lightBright}, {8.85f, lightBright}, {8.97f, 0.f},   {9.1f, 0.f}, {9.11f, lightBright}, {9.15f, lightBright}, {9.27f, 0.f},    {12.f, 0.f}, {12.01f, lightBright}, {12.04f, lightBright}, {12.16f, 0.f} };
std::vector<std::pair<float, float>> zoomAnimTrack = { {9.8f, 1.f}, {10.25f, 1.8f}, {11.25f, 1.8f}, {11.55f, 1.6f},  {13.25f, 1.6f}, {13.6f, 1.f}, {17.5f, 1.f}, {17.51f, 1.4375f} };
std::vector<std::pair<float, float>> smokeAnimTrack = { {0.f, 0.5f}, {3.f, 2.f},{3.4f, 0.5f}, {6.42f, 2.f}, {6.8f, 0.5f}, {9.8f, 2.f}, {10.25f, 0.5f}, {13.25f, 2.f}, {13.7f, 0.5f}, {16.7f, 2.f} };
std::vector<std::pair<float, float>> scenesInterpTrack = { {16.7f, 0.f}, {19.f, 1.f} };
std::vector<std::pair<float, float>> cameraBeTrack = { {19.f, 0.f}, {20.5f, 1.f} };
std::vector<std::pair<float, float>> cloudsFlowTrack = { {18.0f, 1.f}, {25.f, 1.f} };

//--------------------------------------------------------------------------------------------

void renderFrame(int frameN, int subframeFirst, int subframeLast)
{
	float cameraAl = PI;
	float curTime = 0;

	for (int i = 0; i < frameN; ++i) {
		curTime = i / 25.f;
		float cameraAlSpeed = getInterp(cameraAlSpeedTrack, curTime);
		cameraAl += cameraAlSpeed;
	}
	curTime = frameN / 25.f;
	leafScale = getInterp(smokeAnimTrack, curTime);
	zoom = getInterp(zoomAnimTrack, curTime);
	cloudsFlow = getInterp(cloudsFlowTrack, curTime);
	float cameraBe = PI / 4 * (0.5f - 0.5f*cos(PI * getInterp(cameraBeTrack, curTime)));
	scenesInterp = 0.5f - 0.5f*cos(PI * getInterp(scenesInterpTrack, curTime));
	float cameraYOffs = 0;
	time_t timeStart = time(NULL);
	if (scenesInterp <= 0.5f) {
		setupSceneVulcano();
		cameraYOffs = -scenesInterp / 0.5f * SceneSize;
	}
	else {
		setupSceneHeaven();
	}
	setLightning(getInterp(lightningAnimTrack, curTime));
	log1("%d,\n", time(NULL) - timeStart);
	timeStart = time(NULL);
	renderFrameImpl(frameN, cameraAl, cameraBe, cameraYOffs, subframeFirst, subframeLast);
	log2("%d,\n", time(NULL) - timeStart);
}

//--------------------------------------------------------------------------------------------

void renderAnimate()
{
	while (true) {
		int subframeFirst = -1;
		int subframeLast = -1;
		int frameN = 0;
		{
			std::ifstream infile("config.txt");
			int a, b;
			while (infile >> a >> b)
			{
				if (subframeFirst == -1) {
					subframeFirst = a;
					subframeLast = b;
					if (draft) {
						subframeFirst = 0;
						subframeLast = 4;
					}
				}
				else {
					for (int i = a; i <= b; ++i) {
						std::string fname = outPath + FileNamePrefix + digit5intFormat(i) + ".bmp";
						if (!std::experimental::filesystem::exists(fname)) {
							frameN = i;
							goto m1;
						}
					}
				}
			}
		}
		break;
	m1:	renderFrame(frameN, subframeFirst, subframeLast);
	}
}

//--------------------------------------------------------------------------------------------

struct Color
{
	int r = 0;
	int g = 0;
	int b = 0;
	int n = 0;
};

Color colorTab[256];

void initTableByBmp(const std::string& fname)
{
	Color tmpTab[256];

	int xSize = 0;
	int ySize = 0;
	give_bmp_size(fname.c_str(), &xSize, &ySize);
	std::vector<uint8_t> data;
	data.resize(xSize * ySize * 3);
	read_bmp24(fname.c_str(), &data[0]);

	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			int r = data[(y*xSize + x) * 3 + 2];
			int g = data[(y*xSize + x) * 3 + 1];
			int b = data[(y*xSize + x) * 3];
			//int bright = (r * 30 + g * 59 + b * 11) / 100;
			int bright = (r + g + b) / 3;
			//int bright = 
			tmpTab[bright].r += r;
			tmpTab[bright].g += g;
			tmpTab[bright].b += b;
			tmpTab[bright].n++;
		}
	}
	for (int i = 0; i < 256; ++i) {
		if (tmpTab[i].n > 0) {
			tmpTab[i].r /= tmpTab[i].n;
			tmpTab[i].g /= tmpTab[i].n;
			tmpTab[i].b /= tmpTab[i].n;
		}
	}
	tmpTab[0].r = 0;
	tmpTab[0].g = 0;
	tmpTab[0].b = 0;
	tmpTab[0].n = 1;
	for (int i = 255; i >= 0; --i) {
		if (tmpTab[i].n) {
			int maxComponent = tmpTab[i].r;
			if (tmpTab[i].g > maxComponent) {
				maxComponent = tmpTab[i].g;
			}
			if (tmpTab[i].b > maxComponent) {
				maxComponent = tmpTab[i].b;
			}
			tmpTab[255].r = tmpTab[i].r * 255 / maxComponent;
			tmpTab[255].g = tmpTab[i].g * 255 / maxComponent;
			tmpTab[255].b = tmpTab[i].b * 255 / maxComponent;
			tmpTab[255].n = 1;
			break;
		}
	}
	//printf("%d %d %d\n", tmpTab[255].r, tmpTab[255].g, tmpTab[255].b);
	for (int i = 0; i < 256; ++i) {
		if (tmpTab[i].n) {
			colorTab[i] = tmpTab[i];
		}
		else {
			int n1 = i;
			while (tmpTab[n1].n == 0 && n1 > 0)	{
				--n1;
			}
			int n2 = i;
			while (tmpTab[n2].n == 0 && n2 < 255) {
				++n2;
			}
			if (tmpTab[n1].n == 0 && tmpTab[n2].n == 0) {
				exit_msg("color table build error");
			}
			if (tmpTab[n1].n == 0) {
				colorTab[i] = tmpTab[n2];
			}
			else {
				if (tmpTab[n2].n == 0) {
					colorTab[i] = tmpTab[n1];
				}
				else {
					colorTab[i].r = (tmpTab[n2].r - tmpTab[n1].r) * (i - n1) / (n2 - n1) + tmpTab[n1].r;
					colorTab[i].g = (tmpTab[n2].g - tmpTab[n1].g) * (i - n1) / (n2 - n1) + tmpTab[n1].g;
					colorTab[i].b = (tmpTab[n2].b - tmpTab[n1].b) * (i - n1) / (n2 - n1) + tmpTab[n1].b;
				}
			}
		}
	}
	for (int i = 0; i < 256; ++i) {
		char buf[256];
		sprintf(buf, "%d: %d %d %d\n", i, colorTab[i].r, colorTab[i].g, colorTab[i].b);
		log(buf);
	}
}

//--------------------------------------------------------------------------------------------

void colorGradeByTable(const std::string& fname, const std::string& fnameOut)
{
	int xSize = 0;
	int ySize = 0;
	give_bmp_size(fname.c_str(), &xSize, &ySize);
	std::vector<uint8_t> dataIn;
	dataIn.resize(xSize * ySize * 3);
	read_bmp24(fname.c_str(), &dataIn[0]);

	std::vector<uint8_t> dataOut;
	dataOut.resize(xSize * ySize * 3);
	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			int bright = dataIn[(y*xSize + x) * 3];
			if (x > 0 && x < xSize - 1 && y > 0 && y < ySize - 1) {

				bright = dataIn[(y*xSize + x) * 3];
				//bright = dataIn[(y*xSize + x) * 3] +
				//		 dataIn[(y*xSize + x + 1) * 3] +
				//		 dataIn[(y*xSize + x - 1) * 3] +
				//		 dataIn[((y + 1)*xSize + x) * 3] +
				//		 dataIn[((y - 1)*xSize + x) * 3];
				//bright /= 5;
			}
			dataOut[(y*xSize + x) * 3 + 2] = colorTab[bright].r;
			dataOut[(y*xSize + x) * 3 + 1] = colorTab[bright].g;
			dataOut[(y*xSize + x) * 3] = colorTab[bright].b;
			//if (x == 130 && y == 68) {
			//	printf("bright(130,68):%d\n", bright);
			//}
			//if (x == 133 && y == 66) {
			//	printf("bright(133,66):%d\n", bright);
			//}
		}
	}
	save_bmp24(fnameOut.c_str(), xSize, ySize, &dataOut[0]);
}

//--------------------------------------------------------------------------------------------

void colorGrade(const std::string& fname, const std::string& fnameOut, const std::string& fnameColorSample)
{
	initTableByBmp(fnameColorSample);
	colorGradeByTable(fname, fnameOut);
}

//--------------------------------------------------------------------------------------------

void fillScreenBufMaxValue(std::vector<uint8_t>& scrBuffer, uint8_t val)
{
	for (int i = 0; i < (int)scrBuffer.size(); ++i) {
		scrBuffer[i] = std::max(scrBuffer[i], val);
	}
}

//--------------------------------------------------------------------------------------------
void renderAnimateSmokeOfCircles()
{
	int frameN = 0;
	int count = 0;
	std::vector<uint8_t> srcBuffer;
	srcBuffer.resize(ScreenSize * ScreenSize * 3);
	for (int yc=380; yc>30; yc-=2) {
		int r = (int)((420 - yc) / randf(2.7f, 3.2f));
		int xc = rand(200 - r, 200 + r);

		int diametr = (580 - yc) / 10;
		int sqRadius = (diametr / 2) * (diametr / 2);
		for (int fr = 0; fr < 8; ++fr) {            // !!! const 
			for (int i = 0; i < 12; ++i) {       // !!! const 
				int x = rand(0, diametr);
				int y = rand(0, diametr);
				int dx = x - diametr / 2;
				int dy = y - diametr / 2;
				if (dx*dx + dy * dy > sqRadius) {
					--i;
					continue;
				}
				int scrX = x - diametr / 2 + xc;
				int scrY = y - diametr / 2 + yc;
				if (scrX >= 0 && scrX < ScreenSize && scrY >= 0 && scrY < ScreenSize) {
					int pixelOffs = (scrY * ScreenSize + scrX) * 3;
					int bright = rand(50, 170);
					srcBuffer[pixelOffs + 0] = bright;
					srcBuffer[pixelOffs + 1] = bright;
					srcBuffer[pixelOffs + 2] = bright;
				}
			}
		}
		if ((++count) % 4 == 0) {
			std::string fname = "SmokeOfCircles/" + digit5intFormat(frameN++) + ".bmp";
			save_bmp24(fname.c_str(), ScreenSize, ScreenSize, &srcBuffer[0]);
		}
	}
	for (int i = 0; i < 55; ++i) { // !!! const 
		std::string fname = "SmokeOfCircles/" + digit5intFormat(frameN) + ".bmp";
		save_bmp24(fname.c_str(), ScreenSize, ScreenSize, &srcBuffer[0]);
		++frameN;
	}
	fillScreenBufMaxValue(srcBuffer, 170);
	std::string fname = "SmokeOfCircles/" + digit5intFormat(frameN) + ".bmp";
	save_bmp24(fname.c_str(), ScreenSize, ScreenSize, &srcBuffer[0]);
	++frameN;
	fillScreenBufMaxValue(srcBuffer, 255);
	fname = "SmokeOfCircles/" + digit5intFormat(frameN) + ".bmp";
	save_bmp24(fname.c_str(), ScreenSize, ScreenSize, &srcBuffer[0]);
	++frameN;
}

//--------------------------------------------------------------------------------------------

void noiseReduction(const std::string& fname, const std::string& fnameOut)
{
	int xSize = 0;
	int ySize = 0;
	give_bmp_size(fname.c_str(), &xSize, &ySize);
	std::vector<uint8_t> dataIn;
	dataIn.resize(xSize * ySize * 3);
	read_bmp24(fname.c_str(), &dataIn[0]);

	std::vector<uint8_t> dataOut;
	dataOut.resize(xSize * ySize * 3);
	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			int bright = dataIn[(y*xSize + x) * 3];
			if (x > 0 && x < xSize - 1 && y > 0 && y < ySize - 1) {
				float brightF = dataIn[(y*xSize + x) * 3] * 4 +
								dataIn[(y*xSize + x + 1) * 3] +
								dataIn[(y*xSize + x - 1) * 3] +
								dataIn[((y + 1)*xSize + x) * 3] +
								dataIn[((y - 1)*xSize + x) * 3] +
								dataIn[((y + 1)*xSize + x + 1) * 3] * 0.5f +
								dataIn[((y + 1)*xSize + x - 1) * 3] * 0.5f +
								dataIn[((y - 1)*xSize + x + 1) * 3] * 0.5f +
								dataIn[((y - 1)*xSize + x - 1) * 3] * 0.5f;
				brightF /= (4 + 1*4 + 0.5f*4);
				bright = (int)brightF;
			}
			bright = (dataIn[(y*xSize + x) * 3] * 10 + bright * 90) / 100;
			dataOut[(y*xSize + x) * 3 + 2] = bright;
			dataOut[(y*xSize + x) * 3 + 1] = bright;
			dataOut[(y*xSize + x) * 3] = bright;
		}
	}
	save_bmp24(fnameOut.c_str(), xSize, ySize, &dataOut[0]);
}

//--------------------------------------------------------------------------------------------

void correctGamma(const std::string& fname, const std::string& fnameOut)
{
	const double gamma = 0.75f;
	int xSize = 0;
	int ySize = 0;
	give_bmp_size(fname.c_str(), &xSize, &ySize);
	std::vector<uint8_t> dataIn;
	dataIn.resize(xSize * ySize * 3);
	read_bmp24(fname.c_str(), &dataIn[0]);

	std::vector<uint8_t> dataOut;
	dataOut.resize(xSize * ySize * 3);
	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			int bright = dataIn[(y*xSize + x) * 3];
			bright = (int)(pow(bright / 255., gamma) * 255.);
			dataOut[(y*xSize + x) * 3 + 2] = bright;
			dataOut[(y*xSize + x) * 3 + 1] = bright;
			dataOut[(y*xSize + x) * 3] = bright;
		}
	}
	save_bmp24(fnameOut.c_str(), xSize, ySize, &dataOut[0]);
}

//--------------------------------------------------------------------------------------------

/*
	Ray tracing frame 417 subframe 997/1000
	Ray tracing frame 417 subframe 998/1000
	Ray tracing frame 417 subframe 999/1000
	Ray tracing frame 417 subframe 1000/1000
    Noise reduction...  Done
	Gamma correcting...  Done
	Applying color grade...  Done
	All Done!
*/

//--------------------------------------------------------------------------------------------

int main(int argc, char *argv[], char *envp[])
{
	//{
	//	//renderAnimateSmokeOfCircles();

	//	noiseReduction("PostPorcess/Scene00000.bmp", "PostPorcess/Scene00000_Denoise.bmp");
	//	correctGamma("PostPorcess/Scene00000_Denoise.bmp", "PostPorcess/Scene00000_Gamma.bmp");
	//	colorGrade("PostPorcess/Scene00000_Gamma.bmp", "PostPorcess/Scene00000_Color.bmp", "ColorSamples//Ready//colorSample25.bmp");

	//	noiseReduction("PostPorcess/Scene00625.bmp", "PostPorcess/Scene00625_Denoise.bmp");
	//	colorGrade("PostPorcess/Scene00625_Denoise.bmp", "PostPorcess/Scene00625_Color.bmp", "ColorSamples//Ready//colorSample106.bmp");

	//	return 0;
	//}
	
	renderAnimate();
}


