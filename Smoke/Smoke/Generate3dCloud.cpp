#include "pch.h"
#include <iostream>
#include <vector>
#include <functional>
#include <stdarg.h>
#include <algorithm>
#include "bmp.h"
#include "utils.h"
#include "Nodes.h"

//--------------------------------------------------------------------------------------------
// Прототипы
//--------------------------------------------------------------------------------------------
void saveToBmp(const std::string& fileName, int sizeX, int sizeY, std::function<uint8_t(int x, int y)> getPixel);

//--------------------------------------------------------------------------------------------
// Константы
//--------------------------------------------------------------------------------------------

const int BuffersNum = 10;

//--------------------------------------------------------------------------------------------
// Структуры
//--------------------------------------------------------------------------------------------

struct Buffer3D
{
	void reinit(int sizeXYZ_);
	void fillParameters(NodeBase* nodeBase);
	void printParameters();
	void blur(int radius);

	std::shared_ptr<NodeBase> node;
	std::vector<float> cells;
	int sizeXYZ = 0;
	int centerX = 0;
	int centerY = 0;
	int centerZ = 0;
	float innerRadius = 0;
	float outerRadius = 0;
};

//--------------------------------------------------------------------------------------------

struct Object3dToPlace
{
	Object3dToPlace(int x_, int y_, int z_, int srcBuferIndex_, float newOuterRadius_) : x(x_), y(y_), z(z_), srcBuferIndex(srcBuferIndex_), newOuterRadius(newOuterRadius_) {}
	float calcInnerRadius() const;

	int x = 0;                // Позиция центра, куда 
	int y = 0;                //                      будем ставить 
	int z = 0;                //                                    объект

	int srcBuferIndex = 0;    // Индекс Src-буфера, из которого возьмём объект
	float newOuterRadius = 0; // Новый внешний радиус, с которым будет установлен объект
};


//--------------------------------------------------------------------------------------------
// Глобальные переменные
//--------------------------------------------------------------------------------------------

int fullSize = 0;
int halfSize = 0;
static Buffer3D srcBuffers[BuffersNum];
static Buffer3D dstBuffers[BuffersNum];
static std::vector<Object3dToPlace> objects;
NodeBranch object;

//--------------------------------------------------------------------------------------------

void Buffer3D::reinit(int sizeXYZ_)
{
	sizeXYZ = sizeXYZ_;
	centerX = 0;
	centerY = 0;
	centerZ = 0;
	innerRadius = 0;
	outerRadius = 0;

	int cellsNum = sizeXYZ * sizeXYZ * sizeXYZ;
	if (cells.empty()) {
		cells.resize(cellsNum);
	}
	for (int i = 0; i < cellsNum; ++i) {
		cells[i] = 0;
	}
}

//--------------------------------------------------------------------------------------------
void Buffer3D::printParameters()
{
	printf("innerRadius = %f, outerRadius = %f\n", innerRadius, outerRadius);
}

//--------------------------------------------------------------------------------------------

void Buffer3D::blur(int radius)
{
	std::vector<float> tmpBuf;
	tmpBuf.resize(sizeXYZ*sizeXYZ*sizeXYZ);
	for (int z = 0; z < sizeXYZ; ++z) {
		for (int y = 0; y < sizeXYZ; ++y) {
			for (int x = 0; x < sizeXYZ; ++x) {
				float sumBright = 0;
				int num = 0;
				for (int z2 = z-radius; z2 <= z+radius; ++z2) {
					for (int y2 = y-radius; y2 < y+radius; ++y2) {
						for (int x2 = x-radius; x2 < x+radius; ++x2) {
							if (x2 > 0 && x2 < sizeXYZ && y2>0 && y2 < sizeXYZ &&z2>0 && z2 < sizeXYZ) {
								sumBright += cells[z2*sizeXYZ*sizeXYZ + y2*sizeXYZ + x2];
								++num;
							}
						}
					}
				}
				tmpBuf[z*sizeXYZ*sizeXYZ + y * sizeXYZ + x] = sumBright / num;
			}
		}
	}
	cells = tmpBuf;
}

//--------------------------------------------------------------------------------------------


void Buffer3D::fillParameters(NodeBase* nodeBase)
{
	// Найти описанный прямоугольник вокруг картинки в cells
	int x1 = 0;
	int x2 = 0;
	int y1 = 0;
	int y2 = 0;
	int z1 = 0;
	int z2 = 0;

	for (int z = 0; z < halfSize; ++z) {
		for (int y = 0; y < halfSize; ++y) {
			for (int x = 0; x < halfSize; ++x) {
				if (cells[z*halfSize*halfSize + y * halfSize + x] > 0) {
					z1 = z;
					goto m1;
				}
			}
		}
	}
m1:	for (int z = halfSize - 1; z >= 0; --z) {
		for (int y = 0; y < halfSize; ++y) {
			for (int x = 0; x < halfSize; ++x) {
				if (cells[z*halfSize*halfSize + y * halfSize + x] > 0) {
					z2 = z;
					goto m2;
				}
			}
		}
	}

m2:	for (int y = 0; y < halfSize; ++y) {
		for (int z = 0; z < halfSize; ++z) {
			for (int x = 0; x < halfSize; ++x) {
				if (cells[z*halfSize*halfSize + y * halfSize + x] > 0) {
					y1 = y;
					goto m3;
				}
			}
		}
	}
m3:	for (int y = halfSize - 1; y >= 0; --y) {
	for (int z = 0; z < halfSize; ++z) {
		for (int x = 0; x < halfSize; ++x) {
			if (cells[z*halfSize*halfSize + y * halfSize + x] > 0) {
				y2 = y;
				goto m4;
			}
		}
	}
}

m4:	for (int x = 0; x < halfSize; ++x) {
		for (int y = 0; y < halfSize; ++y) {
			for (int z = 0; z < halfSize; ++z) {
				if (cells[z*halfSize*halfSize + y * halfSize + x] > 0) {
					x1 = x;
					goto m5;
				}
			}
		}
	}
m5:	for (int x = halfSize - 1; x >= 0; --x) {
	for (int y = 0; y < halfSize; ++y) {
		for (int z = 0; z < halfSize; ++z) {
			if (cells[z*halfSize*halfSize + y * halfSize + x] > 0) {
				x2 = x;
				goto m6;
			}
		}
	}
}

m6:	centerX = (x1 + x2) / 2;
	centerY = (y1 + y2) / 2;
	centerZ = (z1 + z2) / 2;

	// Заполнить ноду
	nodeBase->bboxX1 = (float)x1;
	nodeBase->bboxX2 = (float)x2;
    nodeBase->bboxY1 = (float)y1;
	nodeBase->bboxY2 = (float)y2;
	nodeBase->bboxZ1 = (float)z1;
	nodeBase->bboxZ2 = (float)z2;
	nodeBase->xCenter = (float)centerX;
	nodeBase->yCenter = (float)centerY;
	nodeBase->zCenter = (float)centerZ;

	// Расчитать максимальную яркость
	float maxBright = 0;

	for (int i = 0; i < halfSize*halfSize*halfSize; ++i) {
		if (cells[i] > maxBright) {
			maxBright = cells[i];
		}
	}

	// Расчитать innerRadius, outerRadius
	innerRadius = FLT_MAX;
	outerRadius = 0;

	for (int z = 0; z < halfSize; ++z) {
		for (int y = 0; y < halfSize; ++y) {
			for (int x = 0; x < halfSize; ++x) {
				float dist = calcDist((float)x, (float)y, (float)z, (float)centerX, (float)centerY, (float)centerZ);
				if (cells[z*halfSize*halfSize + y*halfSize + x] < maxBright * 0.05f) {                                    // !!! const
					if (dist < innerRadius) {
						innerRadius = dist;
					}
				}
				if (cells[z*halfSize*halfSize + y*halfSize + x] > 0) {
					if (dist > outerRadius) {
						outerRadius = dist;
					}
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------

float Object3dToPlace::calcInnerRadius() const
{
	return newOuterRadius / srcBuffers[srcBuferIndex].outerRadius * srcBuffers[srcBuferIndex].innerRadius;
}

//--------------------------------------------------------------------------------------------

void calc3dPosForNewObject(int& bestX_, int& bestY_, int& bestZ_, float innerRadiusOfNewObject, float density, int size)
{
	// Найти центр описанного бокса для всех текущих позиций объектов
	int xMin = INT_MAX;
	int yMin = INT_MAX;
	int zMin = INT_MAX;
	int xMax = INT_MIN;
	int yMax = INT_MIN;
	int zMax = INT_MIN;
	for (const auto& obj : objects) {
		if (obj.x < xMin) xMin = obj.x;
		if (obj.y < yMin) yMin = obj.y;
		if (obj.z < zMin) zMin = obj.z;
		if (obj.x > xMax) xMax = obj.x;
		if (obj.y > yMax) yMax = obj.y;
		if (obj.z > zMax) zMax = obj.z;
	}
	int xCenter = (xMin + xMax) / 2;
	int yCenter = (yMin + yMax) / 2;
	int zCenter = (zMin + zMax) / 2;

	// Выберем интервалы, в которых будем перебирать точки так, чтобы объект формировался примерно по центру рабочей области
	int x1 = 0;
	int x2 = size / 2;
	if (xCenter < size / 2) {
		x1 = size / 2;
		x2 = size;
	}
	int y1 = 0;
	int y2 = size / 2;
	if (yCenter < size / 2) {
		y1 = size / 2;
		y2 = size;
	}
	int z1 = 0;
	int z2 = size / 2;
	if (zCenter < size / 2) {
		z1 = size / 2;
		z2 = size;
	}

	// Перебор точек и выбор лучшей для нового объекта
	int bestX = 0;
	int bestY = 0;
	int bestZ = 0;
	float bestSum = FLT_MAX;

	for (int z = z1; z < z2; ++z) {
		for (int y = y1; y < y2; ++y) {
			for (int x = x1; x < x2; ++x) {
				// Если точка слишком близко хотя бы к одному объекту, то она не подходит
				for (const auto& obj : objects) {
					float dist = calcDist((float)x, (float)y, (float)z, (float)obj.x, (float)obj.y, (float)obj.z);
					if (dist < density*(obj.calcInnerRadius() + innerRadiusOfNewObject)) {
						goto m1;
					}
				}
				// Если точка слишком далеко ото всех объектов, то она не подходит
				int i;
				for (i = 0; i < (int)objects.size(); ++i) {
					const Object3dToPlace& obj = objects[i];
					float dist = calcDist((float)x, (float)y, (float)z, (float)obj.x, (float)obj.y, (float)obj.z);
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
					sum += calcDist((float)x, (float)y, (float)z, (float)obj.x, (float)obj.y, (float)obj.z);                       // !!! Попробовать минимизировать сумму квадратов разностей
				}
				if (sum < bestSum) {
					bestSum = sum;
					bestX = x;
					bestY = y;
					bestZ = z;
				}
			m1:;
			}
		}
	}

	bestX_ = bestX;
	bestY_ = bestY;
	bestZ_ = bestZ;
}

//--------------------------------------------------------------------------------------------

void renderLast3dObjectToDstBufer(std::vector<float>& dstBuffer, int bufSize)
{
	const Object3dToPlace& obj = objects.back();
	const Buffer3D& srcBuffer = srcBuffers[obj.srcBuferIndex];
	                                                                                   // !!! Добавить произовльный угол вращения и другие параметры
    // Расчитать описаный бокс в dstBuffer, куда будем рендерить объект
	int x1 = std::max(obj.x - (int)obj.newOuterRadius - 10, 0);
	int y1 = std::max(obj.y - (int)obj.newOuterRadius - 10, 0);
	int z1 = std::max(obj.z - (int)obj.newOuterRadius - 10, 0);
	int x2 = std::min(obj.x + (int)obj.newOuterRadius + 10, bufSize - 1);
	int y2 = std::min(obj.y + (int)obj.newOuterRadius + 10, bufSize - 1);
	int z2 = std::min(obj.z + (int)obj.newOuterRadius + 10, bufSize - 1);

	float scale = srcBuffer.outerRadius / obj.newOuterRadius;

	for (int z = z1; z <= z2; ++z) {
		for (int y = y1; y <= y2; ++y) {
			for (int x = x1; x <= x2; ++x) {
				int srcX = (int)((x - obj.x) * scale + srcBuffer.centerX);
				int srcY = (int)((y - obj.y) * scale + srcBuffer.centerY);
				int srcZ = (int)((z - obj.z) * scale + srcBuffer.centerZ);
				float srcVal = 0;
				if (srcX >= 0 && srcX < halfSize && srcY >= 0 && srcY < halfSize && srcZ >= 0 && srcZ < halfSize) {
					srcVal = srcBuffer.cells[srcZ*halfSize*halfSize + srcY* halfSize + srcX];
				}
				float dstVal = dstBuffer[z*bufSize*bufSize + y*bufSize + x];
				dstBuffer[z*bufSize*bufSize + y * bufSize + x] = 1 - (1 - srcVal) * (1 - dstVal);
			}
		}
	}
}
//--------------------------------------------------------------------------------------------

void saveCloud(const std::string& fname, std::vector<float>& dst, int size)
{
	FILE* f = fopen(fname.c_str(), "wb");
	if (f) {
		fwrite(&dst[0], size*size*size * sizeof(float), 1, f);
		fclose(f);
	}
}

//--------------------------------------------------------------------------------------------

void load3dCloud(const std::string& fname, std::vector<float>& dst, int size)
{
	FILE* f = fopen(fname.c_str(), "rb");
	if (f) {
		dst.resize(size*size*size);
		fread(&dst[0], size*size*size * sizeof(float), 1, f);
		fclose(f);
	}
}


//--------------------------------------------------------------------------------------------

void generate3dCloudImpl(std::vector<float>& dst, int bufSize)
{
	object.childNodes.clear();
	fullSize = bufSize;
	halfSize = bufSize / 2;
	for (int i = 0; i < BuffersNum; ++i)
	{
		srcBuffers[i].reinit(halfSize);
		dstBuffers[i].reinit(halfSize);
	}

	// Отрендерить шар в нулевой Src буффер
	const float radius = 40;

	for (int z = 0; z < halfSize; ++z) {
		for (int y = 0; y < halfSize; ++y) {
			for (int x = 0; x < halfSize; ++x) {

				float dx = x - halfSize / 2.f;
				float dy = y - halfSize / 2.f;
				float dist = calcDist((float)x, (float)y, (float)z, halfSize / 2.f, halfSize / 2.f, halfSize / 2.f);
				if (dist < radius) {
					float ratio = dist / radius;
					srcBuffers[0].cells[z*halfSize*halfSize + y * halfSize + x] = (cos(ratio * PI) + 1) * 0.5f*0.04f;                                // const !!!
srcBuffers[0].cells[z*halfSize*halfSize + y * halfSize + x] = 1.f;
				}
			}
		}
	}

	std::shared_ptr<NodeBase> pLeaf = std::make_shared<NodeLeaf>();
	srcBuffers[0].fillParameters(pLeaf.get());
	srcBuffers[0].node = pLeaf;

	// Раскопировать по остальным Src буферам
	for (int i = 1; i < BuffersNum; ++i) {
		srcBuffers[i] = srcBuffers[0];
	}

	const int FractalStepNum = 2;                                                                                // const !!!   
	for (int fractalStep = 0; fractalStep < FractalStepNum; ++fractalStep) {
		// Расчитаем параметры и отрендерим большие объекты из маленьких
		for (int bufIndex = 0; bufIndex < BuffersNum; ++bufIndex) {
			printf("bufIndex=%d, fractalStep=%d\n", bufIndex, fractalStep);
			std::shared_ptr<NodeBranch> pBranch = std::make_shared<NodeBranch>();
			objects.clear();
			dstBuffers[bufIndex].reinit(halfSize);
			const int SmallObjectInBig = 90;                                                                     // const !!!
			for (int i = 0; i < SmallObjectInBig; ++i) {
				int srcBuferIndex = rand(0, BuffersNum - 1);
				float newOuterRadius = randf(8.f, 25.f);                                                        // const !!!
				float innerRadiusOfNewObject = newOuterRadius / srcBuffers[srcBuferIndex].outerRadius * srcBuffers[srcBuferIndex].innerRadius;
				if (i == 0) {
					objects.emplace_back(Object3dToPlace(halfSize / 2, halfSize / 2, halfSize / 2, srcBuferIndex, newOuterRadius));
				}
				else {
					int newX = 0;
					int newY = 0;
					int newZ = 0;
					float density = fractalStep == 0 ? 0.4f : 0.5f;                                                                 // const !!! 
					calc3dPosForNewObject(newX, newY, newZ, innerRadiusOfNewObject, density, halfSize);
					objects.emplace_back(Object3dToPlace(newX, newY, newZ, srcBuferIndex, newOuterRadius));
				}
				renderLast3dObjectToDstBufer(dstBuffers[bufIndex].cells, halfSize);
				pBranch->childNodes.emplace_back((float)objects.back().x, (float)objects.back().y, (float)objects.back().z, newOuterRadius / srcBuffers[srcBuferIndex].outerRadius, srcBuffers[srcBuferIndex].node);
			}

			if (fractalStep == 0 && bufIndex == 0)
				saveToBmp("Slices/Buffer0_0.bmp", halfSize, halfSize, [ds = dstBuffers[bufIndex].cells, hs = halfSize](int x, int y) { return (uint8_t)(std::min(ds[(hs - 1 - y) * hs * hs + hs / 2 * hs + x] * 255.f, 255.f)); });
			if (fractalStep == 0 && bufIndex == 1)
				saveToBmp("Slices/Buffer0_1.bmp", halfSize, halfSize, [ds = dstBuffers[bufIndex].cells, hs = halfSize](int x, int y) { return (uint8_t)(std::min(ds[(hs - 1 - y) * hs * hs + hs / 2 * hs + x] * 255.f, 255.f)); });
			if (fractalStep == 1 && bufIndex == 0)
				saveToBmp("Slices/Buffer1_0.bmp", halfSize, halfSize, [ds = dstBuffers[bufIndex].cells, hs = halfSize](int x, int y) { return (uint8_t)(std::min(ds[(hs - 1 - y) * hs * hs + hs / 2 * hs + x] * 255.f, 255.f)); });
			if (fractalStep == 1 && bufIndex == 1)
				saveToBmp("Slices/Buffer1_1.bmp", halfSize, halfSize, [ds = dstBuffers[bufIndex].cells, hs = halfSize](int x, int y) { return (uint8_t)(std::min(ds[(hs - 1 - y) * hs * hs + hs / 2 * hs + x] * 255.f, 255.f)); });

			dstBuffers[bufIndex].fillParameters(pBranch.get());    // Рассчитать параметры сгенерированного изображения, заполнить параметры соответствующей ноды
			dstBuffers[bufIndex].node = pBranch;
		}
		// Скопируем выходные буфера во входные
		for (int i = 0; i < BuffersNum; ++i) {
			srcBuffers[i] = dstBuffers[i];
		}
	}

	// Расчитаем параметры и отрендерим большие объекты из маленьких
	printf("Final pass\n");
	objects.clear();
	dst.resize(fullSize*fullSize*fullSize);
	const int SmallObjectInBig = 90;                                                                     // const !!!
	for (int i = 0; i < SmallObjectInBig; ++i) {
		printf("i = %d\n", i);
		int srcBuferIndex = rand(0, BuffersNum - 1);
		float newOuterRadius = randf(15.f, 50.f);                                                        // const !!!
		float innerRadiusOfNewObject = newOuterRadius / srcBuffers[srcBuferIndex].outerRadius * srcBuffers[srcBuferIndex].innerRadius;
		if (i == 0) {
			objects.emplace_back(Object3dToPlace(fullSize / 2, fullSize / 2, fullSize / 2, srcBuferIndex, newOuterRadius));
		}
		else {
			int newX = 0;
			int newY = 0;
			int newZ = 0;
			calc3dPosForNewObject(newX, newY, newZ, innerRadiusOfNewObject, 0.75f, fullSize);                                // const !!! 0.6
			objects.emplace_back(Object3dToPlace(newX, newY, newZ, srcBuferIndex, newOuterRadius));
		}
//		renderLast3dObjectToDstBufer(dst, fullSize);
		object.childNodes.emplace_back((float)objects.back().x, (float)objects.back().y, (float)objects.back().z, newOuterRadius / srcBuffers[srcBuferIndex].outerRadius, srcBuffers[srcBuferIndex].node);
	}
printf("object.generate3dCloud\n");
object.bboxX2 = (float)bufSize;
object.bboxY2 = (float)bufSize;
object.bboxZ2 = (float)bufSize;
object.generate3dCloud(dst, bufSize);
}

//--------------------------------------------------------------------------------------------

void generate3dCloud(int randSeed, int bufSize)
{
	srand(randSeed);
	std::vector<float> dst;
	generate3dCloudImpl(dst, bufSize);

	int sliceY = 3 * bufSize / 10;
	saveToBmp("Slices/3dCloudSlice_0.bmp", bufSize, bufSize, [dst, sliceY, bufSize](int x, int y) { return (uint8_t)(std::min(dst[(bufSize - 1 - y) * bufSize * bufSize + sliceY * bufSize + x] * 255.f, 255.f)); });
	sliceY = 4 * bufSize / 10;
	saveToBmp("Slices/3dCloudSlice_1.bmp", bufSize, bufSize, [dst, sliceY, bufSize](int x, int y) { return (uint8_t)(std::min(dst[(bufSize - 1 - y) * bufSize * bufSize + sliceY * bufSize + x] * 255.f, 255.f)); });
	sliceY = 5 * bufSize / 10;
	saveToBmp("Slices/3dCloudSlice_2.bmp", bufSize, bufSize, [dst, sliceY, bufSize](int x, int y) { return (uint8_t)(std::min(dst[(bufSize - 1 - y) * bufSize * bufSize + sliceY * bufSize + x] * 255.f, 255.f)); });
	sliceY = 6 * bufSize / 10;
	saveToBmp("Slices/3dCloudSlice_3.bmp", bufSize, bufSize, [dst, sliceY, bufSize](int x, int y) { return (uint8_t)(std::min(dst[(bufSize - 1 - y) * bufSize * bufSize + sliceY * bufSize + x] * 255.f, 255.f)); });
	sliceY = 7 * bufSize / 10;
	saveToBmp("Slices/3dCloudSlice_4.bmp", bufSize, bufSize, [dst, sliceY, bufSize](int x, int y) { return (uint8_t)(std::min(dst[(bufSize - 1 - y) * bufSize * bufSize + sliceY * bufSize + x] * 255.f, 255.f)); });

	saveCloud("Cloud.bin", dst, bufSize);
}

/*
- переключить на новый способ и проверить, что генерит то, что в файлах (старый способ)
- добавить в generate3dCloud параметр hardBrush
- добавить в generate3dCloud параметры блура (делается постэффектом. сначала куб 50х50 заполняется шумом, блурится с неким радиусом (параметр), нормируется и умножается на амплитуду (параметр) затем значения в нём используются, как радиус блура готового облака)




*/
