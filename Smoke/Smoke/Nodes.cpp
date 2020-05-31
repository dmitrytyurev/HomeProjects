#include "pch.h"
#include "Nodes.h"
#include "utils.h"
//--------------------------------------------------------------------------------------------

double NodeBranch::GetDensity(float x, float y, float z, bool isHardBrush)
{
	x += xCenter;  // Переходим из системы координат глифа ноды в систему координат ноды
	y += yCenter;
	z += zCenter;

	if (x < bboxX1 || x > bboxX2 || y < bboxY1 || y > bboxY2 || z < bboxZ1 || z > bboxZ2) {
		return 0.;
	}

	double result = 1.;
	for (const auto& nodeRef: childNodes) {
		result *= 1. - nodeRef.childNode->GetDensity((x - nodeRef.xPos)/nodeRef.scale, (y - nodeRef.yPos) / nodeRef.scale, (z - nodeRef.zPos) / nodeRef.scale, isHardBrush);
	}
	return 1. - result;
}

//--------------------------------------------------------------------------------------------

double NodeLeaf::GetDensity(float x, float y, float z, bool isHardBrush)
{
	const float radius = 40;                                                                                                // const !!!
	float distSq = x * x + y * y + z * z;  // Квадрат расстояние от центра кисти до запрошенной точки

	if (distSq > radius*radius) {
		return 0.;
	}
	if (isHardBrush) {
		return  1.;                                    // Для генерации плотного дыма
	}
	else {
		float dist = sqrt(distSq);
		float ratio = dist / radius;
		return (cos(ratio * PI) + 1) * 0.5 * 0.04;                                                                            // const !!!
	}
}

//--------------------------------------------------------------------------------------------

void NodeBranch::generate3dCloud(std::vector<float>& dst, int bufSize, float xPos, float yPos, float zPos, float scale, bool isHardBrush)
{
	bboxX1 = 0.f;
	bboxY1 = 0.f;
	bboxZ1 = 0.f;                  // !!! Если объект при генерации обрезается гранью z1, то здесь можно задать -100 и перестанет обрезаться
	bboxX2 = (float)bufSize;
	bboxY2 = (float)bufSize;
	bboxZ2 = (float)bufSize;
	xCenter = bufSize / 2.f;
	yCenter = bufSize / 2.f;
	zCenter = bufSize / 2.f;

	dst.resize(bufSize*bufSize*bufSize);

	for (int z = 0; z < bufSize; ++z) {
		printf("z: %d of %d\n", z, bufSize);
		for (int y = 0; y < bufSize; ++y) {
			for (int x = 0; x < bufSize; ++x) {
				dst[z*bufSize*bufSize + y * bufSize + x] = (float)GetDensity((x - xPos)/scale, (y - yPos)/scale, (z - zPos)/scale, isHardBrush);
			}
		}
	}
}