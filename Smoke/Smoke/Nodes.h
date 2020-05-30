#include <memory>
#include <vector>


struct NodeBase;

struct NodeRef
{
	float xPos = 0.f;  // Позиция (в системе координат ноды (ноль в левом верхнем углу)), в которую будет помещена родительская нода (в эту точку будет помещён центр её глифа)
	float yPos = 0.f;
	float zPos = 0.f;
	float scale = 1.f;
	std::shared_ptr<NodeBase> childNode;
};


struct NodeBase
{
	virtual double GetDensity(float x, float y, float z) = 0;  // Дать плотность в точке (в системе координат глифа)

	float xCenter = 0;  // Центр глифа ноды в системе координат ноды 
	float yCenter = 0;
	float zCenter = 0;
	float bboxX1 = 0;   // Ббокс ноды в системе координат ноды
	float bboxY1 = 0;
	float bboxZ1 = 0;
	float bboxX2 = 0;
	float bboxY2 = 0;
	float bboxZ2 = 0;
};

struct NodeBranch: public NodeBase
{
	std::vector<NodeRef> childNodes;

	double GetDensity(float x, float y, float z);  // Дать плотность в точке (в системе координат глифа)
};

struct NodeLeaf : public NodeBase
{
	double GetDensity(float x, float y, float z);  // Дать плотность в точке (в системе координат глифа)
};


