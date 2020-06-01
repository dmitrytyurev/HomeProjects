#include <memory>
#include <vector>
#include <functional>

struct NodeBase;

struct NodeRef
{
	NodeRef() = default;
	NodeRef(float xPos_, float yPos_, float zPos_, float scale_, std::shared_ptr<NodeBase> childNode_) : xPos(xPos_), yPos(yPos_), zPos(zPos_), scale(scale_), childNode(childNode_) {}
	void serialize(FILE* f) const;

	float xPos = 0.f;  // Позиция (в системе координат ноды (ноль в левом верхнем углу)), в которую будет помещена родительская нода (в эту точку будет помещён центр её глифа)
	float yPos = 0.f;
	float zPos = 0.f;
	float scale = 1.f;
	int r = 255;
	int g = 255;
	int b = 255;
	int a = 255;
	std::shared_ptr<NodeBase> childNode;
};


struct NodeBase
{
	virtual double GetDensity(float x, float y, float z, bool isSmoke) = 0;  // Дать плотность в точке (в системе координат глифа)
	virtual void serializeNode(FILE* f, const std::function<bool (NodeBase*)>& isSerializedAlready) = 0;
	void serializeBase(FILE* f);

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

	void serializeNode(FILE* f, const std::function<bool(NodeBase*)>& isSerializedAlready) override;
	double GetDensity(float x, float y, float z, bool isSmoke) override; // Дать плотность в точке (в системе координат глифа)
	void generate3dCloud(std::vector<float>& dst, int bufSize, float xPos, float yPos, float zPos, float scale, bool isSmoke);
	void serializeAll(const std::string& fileName);
};

struct NodeLeaf : public NodeBase
{
	double GetDensity(float x, float y, float z, bool isSmoke);  // Дать плотность в точке (в системе координат глифа)
	void serializeNode(FILE* f, const std::function<bool(NodeBase*)>& isSerializedAlready) override;
};


