// SuperResolution.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <algorithm>
#include <thread>
#include "bmp.h"

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

// Константы ==================================================================================================

const int BORDER = 30;   // На столько же пикселей в каждую сторону пробуем сдвигать блоки для поиска лучшего совпадения
const int REDUCED_BORDER = 5; // // На столько пикселей в каждую сторону пробуем сдвигать блоки относительно найденного на предыдущем шаге сдвига грубым способом
const int SZ_SIZE_DIV = 40; // 30 - Размер квадрата анализа ySize / SZ_SIZE_DIV  
const int NY = 80; // 60 - Количество квадратов анализа по-вертикали
const int WORST_DIFF = 7000; // Если среднее отклонение по пикселю блока превышает это значение, то считаем, что подгонка блока не удалась
const float THESHOLD_RATIO = 0.5f; // Если [diff оптимальной позиции] / [diff угловой точки] больше этого числа, то найденная позиция не является особенной. Пропусти её.
const int THREADS_NUM = 3;


//const int BORDER = 10;   // Отступ от краёв фотографии
//const int SZ_SIZE_DIV = 10; // Размер квадрата анализа ySize / SZ_SIZE_DIV
//const int NY = 10; // Количество квадратов анализа по-вертикали
//const int WORST_DIFF = 7000; // Если среднее отклонение по пикселю блока превышает это значение, то считаем, что подгонка блока не удалась
//const float THESHOLD_RATIO = 0.8f; // Если [diff оптимальной позиции] / [diff угловой точки] больше этого числа, то найденная позиция не является особенной. Пропусти её.


// Типы ==================================================================================================

struct Vect2
{
	Vect2() : x(0), y(0) {}
	Vect2(int x_, int y_) : x(x_), y(y_) {}
	const Vect2& operator+=(const Vect2& a) {
		x += a.x;
		y += a.y;
		return *this;
	}
	const Vect2& operator-=(const Vect2& a) {
		x -= a.x;
		y -= a.y;
		return *this;
	}

	int x, y;
};

#pragma pack(push)
#pragma pack(1)
struct Pixel
{
	Pixel() {}
	Pixel(int r_, int g_, int b_): r(r_), g(g_), b(b_) {}
	unsigned char b, g, r;
};
#pragma pack(pop)

struct Image
{
	Image();
	Image(Image&& src);
	Image(const char* fileName);
	Image(int sizeX_, int sizeY_);
	void operator=(const Image& a) = delete;
	void operator=(Image&& a);
	~Image();

	Pixel* pixels = nullptr;
	int xSize = 0;
	int ySize = 0;
};

Image::Image()
{
}

Image::Image(Image&& a)
{
	pixels = a.pixels;
	xSize = a.xSize;
	ySize = a.ySize;
	a.pixels = nullptr;
}

void Image::operator=(Image&& a)
{
	if (pixels)
		delete[] pixels;
	pixels = a.pixels;
	xSize = a.xSize;
	ySize = a.ySize;
	a.pixels = nullptr;
}

Image::Image(const char* fileName)
{
	give_bmp_size(fileName, &xSize, &ySize);
	pixels = new Pixel[xSize * ySize];
	//int lineSize = xSize * sizeof(Pixel);
	//open_color_bmp(fileName);
	//for (int y = 0; y < ySize; ++y) {
	//	unsigned char*p = give_pixel_adr(0, y, BMP_Y_NORMAL);
	//	memcpy(pixels + xSize * y, p, lineSize);
	//}
	//close_color_bmp();

	read_bmp24(fileName, reinterpret_cast<unsigned char*>(pixels));
}

Image::Image(int xSize_, int ySize_): xSize(xSize_), ySize(ySize_)
{
	pixels = new Pixel[xSize * ySize];
}


Image::~Image()
{
	if (pixels)
		delete []pixels;
}

struct Node
{
	double dx = 0;
	double dy = 0;
	bool filled = 0;
};

struct FindShiftPrm
{
	FindShiftPrm(Pixel* image1_, Pixel* image2_, int regionCornerX_, int regionCornerY_, int boxSizeX_, int boxSizeY_) :
		image1(image1_), image2(image2_), regionCornerX(regionCornerX_), regionCornerY(regionCornerY_), boxSizeX(boxSizeX_), boxSizeY(boxSizeY_)
	{}

	Pixel* image1 = nullptr;
	Pixel* image2 = nullptr;
	int regionCornerX = 0;
	int regionCornerY = 0;
	int boxSizeX = 0;
	int boxSizeY = 0;
};

using ImageRemap = std::vector<std::vector<Node>>;

// Переменные ==================================================================================================

std::vector<Image> images;           // Картинки. Та, что выбрана базовой переместится на нулевое место
ImageRemap imageRemap; // Поля сдвигов всех картинок к базовой. Размер такой же, как images, но нулевой элемент не используется, поскольку сдвиг от базовой картинке к самой себе не нужен

int numberOfImages = 0;
int baseImageNumber = 0;
int xSize = 0, ySize = 0;
int sz = 0;
int nx = 0, ny = 0;
std::vector<int> xPoints;
std::vector<int> yPoints;

// ==================================================================================================


bool IsFileExists(const std::string& name) {
	FILE* file = nullptr;
	fopen_s(&file, name.c_str(), "r");

	if (file) {
		fclose(file);
		return true;
	}
	else {
		return false;
	}
}

double lerp(double val1, double val2, double k) {
	return (val2 - val1) * k + val1;
}

void InitGrid()
{
	sz = ySize / SZ_SIZE_DIV;
	ny = NY;
	nx = ny * xSize / ySize;
	xPoints.resize(nx + 2);
	yPoints.resize(ny + 2);

	yPoints[0] = 0;
	yPoints[ny+1] = ySize-1;
	double ky = double(ySize - 2 * BORDER - sz) / (ny - 1);
	for (int iy=0; iy<ny; ++iy)
	{
		yPoints[iy + 1] = BORDER + int(sz / 2 + iy * ky);
	}

	xPoints[0] = 0;
	xPoints[nx + 1] = xSize - 1;
	double kx = double(xSize - 2 * BORDER - sz) / (nx - 1);
	for (int ix = 0; ix < nx; ++ix)
	{
		xPoints[ix + 1] = BORDER + int(sz / 2 + ix * kx);
	}
}

long long CalcDiff(const FindShiftPrm& p, int dx, int dy)
{
	int* tmp = new int[p.boxSizeX*p.boxSizeY];
	int* tmp2 = new int[p.boxSizeX*p.boxSizeY];
	memset(tmp, 0, sizeof(int)*p.boxSizeX*p.boxSizeY);
	memset(tmp2, 0, sizeof(int)*p.boxSizeX*p.boxSizeY);

	for (int ry = 0; ry < p.boxSizeY; ++ry) {
		int im1yoff = xSize*(p.regionCornerY + ry);
		int im2yoff = xSize*(p.regionCornerY + ry + dy);
		for (int rx = 0; rx < p.boxSizeX; ++rx) {
			Pixel* p1 = p.image1 + im1yoff + (p.regionCornerX + rx);
			Pixel* p2 = p.image2 + im2yoff + (p.regionCornerX + rx + dx);
			int d = (p1->r - p2->r)*(p1->r - p2->r) + (p1->g - p2->g)*(p1->g - p2->g) + (p1->b - p2->b)*(p1->b - p2->b);
			tmp[ry*p.boxSizeX + rx] = d;
		}
	}

	for (int y = 0; y < p.boxSizeY; ++y) {
		for (int x = 0; x < p.boxSizeX; ++x) {
			int d = tmp[y*p.boxSizeX + x];

			if (x < p.boxSizeX - 1) {
				int dn = tmp[y*p.boxSizeX + x + 1];
				if (dn < d)
					d = dn;
			}

			if (y < p.boxSizeY - 1) {
				int dn = tmp[(y + 1)*p.boxSizeX + x];
				if (dn < d)
					d = dn;
			}

			tmp2[y*p.boxSizeX + x] = d;
		}
	}

	long long curDiff = 0;
	for (int y = 0; y < p.boxSizeY; ++y) {
		for (int x = 0; x < p.boxSizeX; ++x) {
			curDiff += tmp2[y*p.boxSizeX + x];
		}
	}
	
	delete[] tmp;
	delete[] tmp2;

	return curDiff;
}

long long CalcDiffRobust(const FindShiftPrm& p, int dx, int dy)
{
	int halfSizeX = (p.boxSizeX + 1) / 2;
	int halfSizeY = (p.boxSizeY + 1) / 2;

	int* tmp = new int[halfSizeX*halfSizeY];
	int* tmp2 = new int[halfSizeX*halfSizeY];
	memset(tmp, 0, sizeof(int)*halfSizeX*halfSizeY);
	memset(tmp2, 0, sizeof(int)*halfSizeX*halfSizeY);

	for (int ry = 0; ry < p.boxSizeY; ry += 2) {
		int im1yoff = xSize * (p.regionCornerY + ry);
		int im2yoff = xSize * (p.regionCornerY + ry + dy);
		for (int rx = 0; rx < p.boxSizeX; rx += 2) {
			Pixel* p1 = p.image1 + im1yoff + (p.regionCornerX + rx);
			Pixel* p2 = p.image2 + im2yoff + (p.regionCornerX + rx + dx);
			int d = (p1->r - p2->r)*(p1->r - p2->r) + (p1->g - p2->g)*(p1->g - p2->g) + (p1->b - p2->b)*(p1->b - p2->b);
			tmp[ry/2* halfSizeX + rx/2] = d;
		}
	}

	for (int y = 0; y < halfSizeY; ++y) {
		for (int x = 0; x < halfSizeX; ++x) {
			int d = tmp[y*halfSizeX + x];

			if (x < halfSizeX - 1) {
				int dn = tmp[y*halfSizeX + x + 1];
				if (dn < d)
					d = dn;
			}

			if (y < halfSizeY - 1) {
				int dn = tmp[(y + 1)*halfSizeX + x];
				if (dn < d)
					d = dn;
			}

			tmp2[y*halfSizeX + x] = d;
		}
	}

	long long curDiff = 0;
	for (int y = 0; y < halfSizeY; ++y) {
		for (int x = 0; x < halfSizeX; ++x) {
			curDiff += tmp2[y*halfSizeX + x];
		}
	}

	delete[] tmp;
	delete[] tmp2;

	return curDiff;
}


Node FindBestShift(const FindShiftPrm& p)
{
	long long minDiff = LLONG_MAX;                          // Сначала грубый поиск
	int bestDxRobust = 0;
	int bestDyRobust = 0;

	for (int dy = -BORDER; dy <= BORDER; dy += 2) {        
		for (int dx = -BORDER; dx <= BORDER; dx += 2) {
			long long curDiff = CalcDiffRobust(p, dx, dy);
			if (curDiff < minDiff) {
				minDiff = curDiff;
				bestDxRobust = dx;
				bestDyRobust = dy;
			}
		}
	}

	minDiff = LLONG_MAX;                                // Теперь уточняющий поиск в локальной зоне
	int bestDx = 0;
	int bestDy = 0;
	int dxFrom = std::max(bestDxRobust - REDUCED_BORDER, -BORDER);
	int dxTo   = std::min(bestDxRobust + REDUCED_BORDER, BORDER);
	int dyFrom = std::max(bestDyRobust - REDUCED_BORDER, -BORDER);
	int dyTo   = std::min(bestDyRobust + REDUCED_BORDER, BORDER);
	for (int dy = dyFrom; dy <= dyTo; ++dy) {
		for (int dx = dxFrom; dx <= dxTo; ++dx) {
			long long curDiff = CalcDiff(p, dx, dy);
			if (curDiff < minDiff) {
				minDiff = curDiff;
				bestDx = dx;
				bestDy = dy;
			}
		}
	}
	//printf("%I64d %d %d === Robust: %d %d\n", minDiff, bestDx, bestDy, bestDxRobust, bestDyRobust);


	//int bestDx = 0;
	//int bestDy = 0;
	//for (int dy = -BORDER; dy <= BORDER; dy += 1) {
	//	for (int dx = -BORDER; dx <= BORDER; dx += 1) {
	//		long long curDiff = CalcDiff(p, dx, dy);
	//		if (curDiff < minDiff) {
	//			minDiff = curDiff;
	//			bestDx = dx;
	//			bestDy = dy;
	//		}
	//	}
	//}
	//printf("%I64d %d %d\n", minDiff, bestDx, bestDy);

	long long worstDiffForBlock = WORST_DIFF * p.boxSizeX * p.boxSizeY;
	if (minDiff > worstDiffForBlock) {
		return Node();
	}

	std::vector<Vect2> pointsToCheck;  // Возьмём несколько удалённых сдвигов от лучшего найденного. Если хотя бы в одном недостаточно большая разница с лучшим, то считаем, что найденный сдвиг неуникален и невозвращаем его.
	if (bestDx < 0) {
		if (bestDy < 0) {
			pointsToCheck.emplace_back(BORDER, -BORDER);
			pointsToCheck.emplace_back(-BORDER, BORDER);
			pointsToCheck.emplace_back(BORDER, BORDER);
			pointsToCheck.emplace_back(BORDER, 0);
			pointsToCheck.emplace_back(0, BORDER);
		} else {
			pointsToCheck.emplace_back(-BORDER, -BORDER);
			pointsToCheck.emplace_back(BORDER, -BORDER);
			pointsToCheck.emplace_back(BORDER, BORDER);
			pointsToCheck.emplace_back(BORDER, 0);
			pointsToCheck.emplace_back(0, -BORDER);
		}
	} else
	{
		if (bestDy < 0) {
			pointsToCheck.emplace_back(-BORDER, -BORDER);
			pointsToCheck.emplace_back(-BORDER, BORDER);
			pointsToCheck.emplace_back(BORDER, BORDER);
			pointsToCheck.emplace_back(-BORDER, 0);
			pointsToCheck.emplace_back(0, BORDER);
		} else {
			pointsToCheck.emplace_back(-BORDER, -BORDER);
			pointsToCheck.emplace_back(-BORDER, BORDER);
			pointsToCheck.emplace_back(BORDER, -BORDER);
			pointsToCheck.emplace_back(-BORDER, 0);
			pointsToCheck.emplace_back(0, -BORDER);
		}
	}

	for (const auto& point : pointsToCheck) {
		long long curDiff = CalcDiff(p, point.x, point.y);
		if (curDiff == 0) {
			return Node();
		}
		double ratio = (double)minDiff / (double)curDiff;
		if (ratio > THESHOLD_RATIO)
			return Node();
	}

	Node node;
	node.dx = bestDx;
	node.dy = bestDy;
	node.filled = true;
	return node;
}

int FindPointX(int val)
{
	int n = 0;
	while (n < nx && xPoints[n] <= val)
		++n;
	return n;
}

int FindPointY(int val)
{
	int n = 0;
	while (n < ny && yPoints[n] <= val)
		++n;
	return n;
}

void FindBestFit(int ix, int xEnd, int iy)
{
	if (ix >= xEnd)
		return;

	ImageRemap& curMap = imageRemap;
	// Рассчитать левый верхний угол и размер анализируемого блока
	int cornerX = xPoints[ix + 1] - int(sz / 2);
	int cornerY = yPoints[iy + 1] - int(sz / 2);
	int boxSizeX = sz;
	int boxSizeY = sz;
	int over = cornerX + boxSizeX + BORDER - xSize;
	if (over > 0)
		boxSizeX -= over;
	over = cornerY + boxSizeY + BORDER - ySize;
	if (over > 0)
		boxSizeY -= over;
	// Рассчитать подбором оптимальный сдвиг блока
	curMap[ix + 1][iy + 1] = FindBestShift(FindShiftPrm(images[0].pixels, images[1].pixels, cornerX, cornerY, boxSizeX, boxSizeY));

//printf("x1:%d y1:%d - x2:%d y2:%d = %f %f %d\n", cornerX, cornerY, cornerX+boxSizeX, cornerY + boxSizeY, curMap[ix + 1][iy + 1].dx, curMap[ix + 1][iy + 1].dy, int(curMap[ix + 1][iy + 1].filled)); //Switch!!!
}

void FillGridNodesForImg(int iImg)
{
	//int x1 = 2094;  //Switch!!!
	//int y1 = 3488;
	//int x2 = 10937;
	//int y2 = 7952;
	//int xBegin = FindPointX(x1) - 2;
	//if (xBegin < 0) xBegin = 0;
	//int xEnd = FindPointX(x2);
	//int yBegin = FindPointY(y1) - 2;
	//if (yBegin < 0) yBegin = 0;
	//int yEnd = FindPointY(y2);

	int xBegin = 0;
	int xEnd = nx;
	int yBegin = 0;
	int yEnd = ny;

	int blocksNum = ((xEnd - xBegin) + (THREADS_NUM - 1)) / THREADS_NUM;      // Вариант с потоками
	for (int iy = yBegin; iy < yEnd; ++iy) {
		printf("   img:%d, progress: %d%%\n", iImg, iy * 100 / (yEnd - 1));
		for (int ib = 0; ib < blocksNum; ++ib) {
			std::thread thread1(FindBestFit, ib*THREADS_NUM + xBegin + 0, xEnd, iy);
			std::thread thread2(FindBestFit, ib*THREADS_NUM + xBegin + 1, xEnd, iy);
			std::thread thread3(FindBestFit, ib*THREADS_NUM + xBegin + 2, xEnd, iy);
			thread1.join();
			thread2.join();
			thread3.join();
		}
	}

	//for (int iy = yBegin; iy < yEnd; ++iy) {                  // Вариант без потоков
	//	printf("img:%d, iy:%d till %d\n", iImg, iy, yEnd);
	//	for (int ix = xBegin; ix < xEnd; ++ix) {
	//		FindBestFit(iImg, ix, iy);
	//	}
	//}

	// Заполнить краевые и пропущенные ноды интерполяцией
	struct NEIGHB
	{
		int dx = 0;
		int dy = 0;
		double weight = 0;
	};
	const double cornerWeight = 1.0 / sqrt(2.0);
	std::vector<NEIGHB> neighbs = { {-1, -1, cornerWeight}, {0, -1, 1}, {1, -1, cornerWeight}, {1, 0, 1}, {1, 1, cornerWeight}, {0, 1, 1}, {-1, 1, cornerWeight}, {-1, 0, 1}};

	ImageRemap& curMap = imageRemap;
	while (true) // Цикл итераций - расходяейся волной заполняем ноды от уже заполненных
	{
		int unfilledCount = 0;
		int newFilledCount = 0;
		ImageRemap newMap;
		newMap.resize(nx + 2);
		for (int i = 0; i < nx + 2; ++i) {
			newMap[i].resize(ny + 2);
		}

		for (int y = 0; y < ny + 2; ++y) {
			for (int x = 0; x < nx + 2; ++x) {
				if (curMap[x][y].filled) {
					newMap[x][y] = curMap[x][y];
					continue;
				}

				// Читаем из curMap, пишем в newMap

				double sumDx = 0;
				double sumDy = 0;
				double sumWeight = 0;
				for (const auto& neighb : neighbs) {
					int xNeighb = x + neighb.dx;
					int yNeighb = y + neighb.dy;
					if (xNeighb < 0 || xNeighb == nx + 2 || yNeighb < 0 || yNeighb == ny + 2) {
						continue;
					}
					const Node& curNeighb = curMap[xNeighb][yNeighb];
					if (!curNeighb.filled) {
						continue;
					}
					sumWeight += neighb.weight;
					sumDx += neighb.weight * curNeighb.dx;
					sumDy += neighb.weight * curNeighb.dy;
				}
				if (sumWeight == 0) {
					++unfilledCount;
				} else {
					++newFilledCount;
					newMap[x][y].filled = true;
					newMap[x][y].dx = sumDx / sumWeight;
					newMap[x][y].dy = sumDy / sumWeight;
				}
			}
		}
		curMap = newMap;
		if (unfilledCount == 0)
			break;
		if (newFilledCount == 0) {
			printf("Can't fill all nodes for img %d\n", iImg);
			exit(1);
		}
	}
}


int FindBaseImage()
{
	puts("Calculating best base image");
	if (images.size() < 3) {
		printf("   Done! It's %d image\n", 0);
		return 0;
	}

	struct ImageDiff
	{
		ImageDiff(int image1Index_, int image2Index_, long long diff_) : image1Index(image1Index_), image2Index(image2Index_), diff(diff_) {}
		int image1Index = 0;
		int image2Index = 0;
		long long diff = 0;
	};

	std::vector<ImageDiff> imageDiffs;                      // Считаем попарные Diff'ы между картинками
	for (int i = 0; i < (int)images.size()-1; ++i) {
		printf("   progress: %d%%\n", i * 100 / ((int)images.size() - 2));
		for (int i2 = i+1; i2 < (int)images.size(); ++i2) {
			long long curDiff = 0;
			Pixel* image1 = images[i].pixels;
			Pixel* image2 = images[i2].pixels;

			for (int y = 0; y < ySize / 2; ++y) {
				int imYoff = xSize * (y * 2);
				for (int x = 0; x < xSize / 2; ++x) {
					Pixel* p1 = image1 + imYoff + x * 2;
					Pixel* p2 = image2 + imYoff + x * 2;
					curDiff += (p1->r - p2->r)*(p1->r - p2->r) + (p1->g - p2->g)*(p1->g - p2->g) + (p1->b - p2->b)*(p1->b - p2->b);
				}
			}
			imageDiffs.emplace_back(i, i2, curDiff);
		}
	}

	long long imageDiffsSumMin = LLONG_MAX;
	int bestImageIndex = -1;
	for (int i = 0; i < (int)images.size(); ++i) {           // Считаем сумму Diff'ов от каждой картинки до всех остальных. Базовая картинка - у кот. эта сумма меньше.
		long long imageDiffsSum = 0;
		for (int i2 = 0; i2 < (int)images.size(); ++i2) {
			if (i == i2)
				continue;
			int n = i;
			int n2 = i2;
			if (n > n2) {
				std::swap(n, n2);
			}
			for (const auto& curDiff: imageDiffs) {
				if (curDiff.image1Index == n && curDiff.image2Index == n2) {
					imageDiffsSum += curDiff.diff;
					break;
				}
			}


		}
		if (imageDiffsSum < imageDiffsSumMin) {
			imageDiffsSumMin = imageDiffsSum;
			bestImageIndex = i;
		}
	}

	printf("   Done! It's %d image\n", bestImageIndex);
	return bestImageIndex;
}

void RemapImage()
{
	Image img(xSize, ySize); 
	ImageRemap& curMap = imageRemap;
	Pixel* srcPixels = images[1].pixels;
	Pixel* dstPixels = img.pixels;

//for (int iy = 0; iy < ny + 2; iy++) {
//	for (int ix = 0; ix < nx + 2; ix++) {
//		curMap[ix][iy].dx = 0;
//		curMap[ix][iy].dy = 0;
//	}
//}
//curMap[5][5].dx = 10;
//curMap[5][5].dy = 10;


	for (int iy = 0; iy < ny + 1; iy++) {
		for (int ix = 0; ix < nx + 1; ix++) {
			for (int y = yPoints[iy]; y <= yPoints[iy+1]; y++) {
				double k = double(y - yPoints[iy]) / (yPoints[iy + 1] - yPoints[iy]);
				double dxLeft  = lerp(curMap[ix][iy].dx, curMap[ix][iy + 1].dx, k);
				double dyLeft  = lerp(curMap[ix][iy].dy, curMap[ix][iy + 1].dy, k);
				double dxRight = lerp(curMap[ix + 1][iy].dx, curMap[ix + 1][iy + 1].dx, k);
				double dyRight = lerp(curMap[ix + 1][iy].dy, curMap[ix + 1][iy + 1].dy, k);
				for (int x = xPoints[ix]; x <= xPoints[ix + 1]; x++) {
					double k = double(x - xPoints[ix]) / (xPoints[ix + 1] - xPoints[ix]);
					double dx = lerp(dxLeft, dxRight, k);
					double dy = lerp(dyLeft, dyRight, k);

					int srcX = x + int(dx + 0.5f);
					if (srcX < 0)
						srcX = 0;
					else
						if (srcX >= xSize)
							srcX = xSize - 1;

					int srcY = y + int(dy + 0.5f);
					if (srcY < 0)
						srcY = 0;
					else
						if (srcY >= ySize)
							srcY = ySize - 1;

					dstPixels[y*xSize + x] = srcPixels[srcY*xSize + srcX];
				}
			}
		}
	}
	images[1] = std::forward<Image>(img);
}

void SaveResult()
{

}


void PrintDiff(int x1, int y1, int x2, int y2, int image1, int image2, long long* diffOld, long long* diffNew, int dx, int dy)
{
	int* tmp = new int[xSize*ySize];
	int* tmp2 = new int[xSize*ySize];
	for (int i = 0; i < xSize*ySize; ++i) {
		tmp[i] = 0;
		tmp2[i] = 0;
	}

	long long curDiffOld= 0;

	for (int y = y1; y < y2; ++y) {
		int im1yoff = xSize * y;
		int im2yoff = xSize * (y + dy);
		for (int x = x1; x < x2; ++x) {
			Pixel* p1 = images[image1].pixels + im1yoff + x;
			Pixel* p2 = images[image2].pixels + im2yoff + x + dx;
			int d = (p1->r - p2->r)*(p1->r - p2->r) + (p1->g - p2->g)*(p1->g - p2->g) + (p1->b - p2->b)*(p1->b - p2->b);
			curDiffOld += d;
			tmp[y*xSize + x] = d;
		}
	}
	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			int d = tmp[y*xSize + x];

			if (x < xSize-1) {
				int dn = tmp[y*xSize + x + 1];
				if (dn < d)
					d = dn;
			}

			if (x > 0) {
				int dn = tmp[y*xSize + x - 1];
				if (dn < d)
					d = dn;
			}

			if (y < ySize - 1) {
				int dn = tmp[(y+1)*xSize + x];
				if (dn < d)
					d = dn;
			}

			if (y > 0) {
				int dn = tmp[(y-1)*xSize + x];
				if (dn < d)
					d = dn;
			}
			tmp2[y*xSize + x] = d;
		}
	}

	long long curDiffNew = 0;
	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			curDiffNew += tmp2[y*xSize + x];
		}
	}
	*diffOld = curDiffOld;
	*diffNew = curDiffNew;
	delete[] tmp;
	delete[] tmp2;
}

void DiffTest(const char* name1, const char* name2)
{
	images.emplace_back(name1);
	images.emplace_back(name2);
	xSize = images[0].xSize;
	ySize = images[0].ySize;

	long long diffOld, diffNew;

	int x1 = 6495;
	int y1 = 5221;
	int x2 = x1 + 302;
	int y2 = y1 + 302;
	PrintDiff(x1, y1, x2, y2, 0, 1, &diffOld, &diffNew, 0, 0);
	printf("Diff: %I64d %I64d\n", diffOld, diffNew);
	PrintDiff(x1, y1, x2, y2, 0, 1, &diffOld, &diffNew, 1, 0);
	printf("Diff: %I64d %I64d\n", diffOld, diffNew);
	PrintDiff(x1, y1, x2, y2, 0, 1, &diffOld, &diffNew, -1, 0);
	printf("Diff: %I64d %I64d\n", diffOld, diffNew);
	PrintDiff(x1, y1, x2, y2, 0, 1, &diffOld, &diffNew, 0, 1);
	printf("Diff: %I64d %I64d\n", diffOld, diffNew);
	PrintDiff(x1, y1, x2, y2, 0, 1, &diffOld, &diffNew, 0, -1);
	printf("Diff: %I64d %I64d\n", diffOld, diffNew);
}

Pixel CombinePixelAverage(int pixelIndex)
{
	int r = 0;
	int g = 0;
	int b = 0;
	int imagesNum = (int)images.size();
	for (int iImg = 0; iImg < images.size(); ++iImg) {
		const Pixel& curPixel = images[iImg].pixels[pixelIndex];
		r += curPixel.r;
		g += curPixel.g;
		b += curPixel.b;
	}
	r /= imagesNum;
	g /= imagesNum;
	b /= imagesNum;

	return Pixel(r, g, b);
}

Pixel CombinePixelMedian(int pixelIndex)
{
	static std::vector<int> comp;
	Pixel p1(0, 0, 0);
	Pixel p2(0, 0, 0);
	int index1 = ((int)comp.size() - 1) / 2;
	int index2 = ((int)comp.size()) / 2;

	for (int i = 0; i < 3; ++i) {
		comp.clear();
		for (int iImg = 0; iImg < images.size(); ++iImg) {
			const unsigned char* pc = ((const unsigned char*)(images[iImg].pixels + pixelIndex)) + i;
			comp.push_back(*pc);
		}
		std::sort(comp.begin(), comp.end());
		((unsigned char*)&p1)[i] = comp[index1];
		((unsigned char*)&p2)[i] = comp[index2];
	}
	return Pixel((int(p1.r) + p2.r) / 2, (int(p1.g) + p2.g) / 2, (int(p1.b) + p2.b) / 2);
}

Pixel CombinePixelMostClose(int pixelIndex)
{
	long long pixelDiffsSumMin = LLONG_MAX;
	int bestPixelIndex = -1;
	for (int i = 0; i < (int)images.size(); ++i) {           // Считаем сумму Diff'ов от каждой картинки до всех остальных. Базовая картинка - у кот. эта сумма меньше.
		long long pixelDiffsSum = 0;
		const Pixel& p1 = images[i].pixels[pixelIndex];
		for (int i2 = 0; i2 < (int)images.size(); ++i2) {
			if (i == i2)
				continue;

			const Pixel& p2 = images[i2].pixels[pixelIndex];
			int diff = (p1.r - p2.r)*(p1.r - p2.r) + (p1.g - p2.g)*(p1.g - p2.g) + (p1.b - p2.b)*(p1.b - p2.b);
			pixelDiffsSum += diff;
		}
		if (pixelDiffsSum < pixelDiffsSumMin) {
			pixelDiffsSumMin = pixelDiffsSum;
			bestPixelIndex = i;
		}
	}

	return images[bestPixelIndex].pixels[pixelIndex];
}

void ProcessImage(int iImg)
{
	FillGridNodesForImg(iImg);
	RemapImage();
	char fname[] = "0_aligned.bmp";
	fname[0] = iImg + '0';

	save_bmp24(fname, xSize, ySize, (const char*)(images[1].pixels));

	//for (int y = 0; y < ny + 2; ++y) {
	//	for (int x = 0; x < nx + 2; ++x) {
	//		ImageRemap& ir = imageRemaps[iImg];
	//		printf("%5.2f %5.2f %d  ", ir[x][y].dx, ir[x][y].dy, ir[x][y].filled);
	//	}
	//	printf("\n");
	//}

}

void CombineImages()
{
	puts("Loading combined images");
	std::string fname = "0_aligned.bmp";
	for (int i = 0; i < numberOfImages; ++i) {
		if (i == baseImageNumber)
			continue;
		fname[0] = i + '0';
		images.emplace_back(fname.c_str());
		printf("   %s loaded\n", fname.c_str());
	}

	puts("Combining aligned images");

	Image result(xSize, ySize);
	for (int y = 0; y < ySize; ++y)	{
		if ((y % 300) == 0 || y == ySize - 1)
			printf("   combining progress: %d%%\n", y * 100 / (ySize - 1));
		for (int x = 0; x < xSize; ++x)	{
			int pixelIndex = y * xSize + x;
			result.pixels[pixelIndex] = CombinePixelMedian(pixelIndex);
		}
	}
	printf("   Done!\n");

	puts("Saving result");
	save_bmp24("result.bmp", xSize, ySize, (const char*)(result.pixels));
	printf("   Done!\n");
}

void LoadImageFiles()
{
	puts("Loading images");
	std::string fname = "0.bmp";
	int n = 0;

	while (true)
	{
		if (!IsFileExists(fname))
			break;
		images.emplace_back(fname.c_str());
		printf("   %s loaded\n", fname.c_str());
		++n;
		fname[0] = n + '0';
	}

	if (images.size() < 2) {
		puts("Should be two or more files: 0.bmp, 1.bmp, ...");
		exit(0);
	}

	numberOfImages = n;
	xSize = images[0].xSize;
	ySize = images[0].ySize;
}

void AlignImages()
{
	puts("Aligning images");
	InitGrid();
	for (int i = 0; i < numberOfImages; ++i) {
		if (i == baseImageNumber)
			continue;
		std::string fnameAligned = "0_aligned.bmp";
		fnameAligned[0] = i + '0';
		if (IsFileExists(fnameAligned))
			continue;

		std::string fname = "0.bmp";
		fname[0] = i + '0';
		images.emplace_back(fname.c_str());

		imageRemap.clear();
		imageRemap.resize(nx + 2);
		for (int i = 0; i < nx + 2; ++i) {
			imageRemap[i].resize(ny + 2);
		}

		ProcessImage(i);
		images.resize(1);
	}
}

int main()
{
	LoadImageFiles();
	baseImageNumber = FindBaseImage();   // Найдём индекс базовой картинки. Остальные слои будем сдвигать до соответствия с ней.
	if (baseImageNumber)
		std::swap(images[0], images[baseImageNumber]);
	images.resize(1);
	AlignImages();
	CombineImages(); 
}



