#include "pch.h"
#include "Nodes.h"
#include "utils.h"
//--------------------------------------------------------------------------------------------

double NodeBranch::GetDensity(float x, float y, float z)
{
	x += xCenter;  // Переходим из системы координат глифа ноды в систему координат ноды
	y += yCenter;
	z += zCenter;

	if (x < bboxX1 || x > bboxX2 || y < bboxY1 || y > bboxY2 || z < bboxZ1 || z > bboxZ2) {
		return 0.;
	}

	double result = 1.;
	for (const auto& nodeRef: childNodes) {
		result *= 1. - nodeRef.childNode->GetDensity((x - nodeRef.xPos)/nodeRef.scale, (y - nodeRef.yPos) / nodeRef.scale, (z - nodeRef.zPos) / nodeRef.scale);
	}
	return 1. - result;
}

//--------------------------------------------------------------------------------------------

double NodeLeaf::GetDensity(float x, float y, float z)
{
	const float radius = 40;                                                                                                // const !!!
	float distSq = x * x + y * y + z * z;  // Квадрат расстояние от центра кисти до запрошенной точки

	if (distSq > radius*radius) {
		return 0.;
	}
	float dist = sqrt(distSq);
	float ratio = dist / radius;
//	return (cos(ratio * PI) + 1) * 0.5 * 0.04;                                                                            // const !!!
return  1.;                                    // Для генерации туч
}

//--------------------------------------------------------------------------------------------

void NodeBranch::generate3dCloud(std::vector<float>& dst, int bufSize)
{
	dst.resize(bufSize*bufSize*bufSize);

	for (int z = 0; z < bufSize; ++z) {
		printf("z: %d\n", z);
		for (int y = 0; y < bufSize; ++y) {
			for (int x = 0; x < bufSize; ++x) {
				dst[z*bufSize*bufSize + y * bufSize + x] = (float)GetDensity((float)x, (float)y, (float)z);
			}
		}
	}
}