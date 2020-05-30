#include "pch.h"
#include "Nodes.h"
#include "utils.h"
//--------------------------------------------------------------------------------------------

double NodeBranch::GetDensity(float x, float y, float z)
{
	x += xCenter;  // ��������� �� ������� ��������� ����� ���� � ������� ��������� ����
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
	float distSq = x * x + y * y + z * z;  // ������� ���������� �� ������ ����� �� ����������� �����

	if (distSq > radius*radius) {
		return 0.;
	}
	float dist = sqrt(distSq);
	float ratio = dist / radius;
	return (cos(ratio * PI) + 1) * 0.5 * 0.04;                                                                            // const !!!
   //return  1.;                                    // ��� ��������� ���
}
