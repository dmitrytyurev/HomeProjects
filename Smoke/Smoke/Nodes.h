#include <memory>
#include <vector>


struct NodeBase;

struct NodeRef
{
	float xPos = 0.f;  // ������� (� ������� ��������� ���� (���� � ����� ������� ����)), � ������� ����� �������� ������������ ���� (� ��� ����� ����� ������� ����� � �����)
	float yPos = 0.f;
	float zPos = 0.f;
	float scale = 1.f;
	std::shared_ptr<NodeBase> childNode;
};


struct NodeBase
{
	virtual double GetDensity(float x, float y, float z) = 0;  // ���� ��������� � ����� (� ������� ��������� �����)

	float xCenter = 0;  // ����� ����� ���� � ������� ��������� ���� 
	float yCenter = 0;
	float zCenter = 0;
	float bboxX1 = 0;   // ����� ���� � ������� ��������� ����
	float bboxY1 = 0;
	float bboxZ1 = 0;
	float bboxX2 = 0;
	float bboxY2 = 0;
	float bboxZ2 = 0;
};

struct NodeBranch: public NodeBase
{
	std::vector<NodeRef> childNodes;

	double GetDensity(float x, float y, float z);  // ���� ��������� � ����� (� ������� ��������� �����)
};

struct NodeLeaf : public NodeBase
{
	double GetDensity(float x, float y, float z);  // ���� ��������� � ����� (� ������� ��������� �����)
};


