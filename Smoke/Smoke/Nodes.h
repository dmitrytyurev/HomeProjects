#include <memory>
#include <vector>
#include <functional>
#include <map>

struct NodeBase;

using MapRef = std::map<uint64_t, std::shared_ptr<NodeBase>>;

struct NodeRef
{
	NodeRef() = default;
	NodeRef(float xPos_, float yPos_, float zPos_, float scale_, std::shared_ptr<NodeBase> childNode_) : xPos(xPos_), yPos(yPos_), zPos(zPos_), scale(scale_), childNode(childNode_) {}
	void serialize(FILE* f) const;
	void deserialize(FILE* f);

	float xPos = 0.f;  // ������� (� ������� ��������� ���� (���� � ����� ������� ����)), � ������� ����� �������� ������������ ���� (� ��� ����� ����� ������� ����� � �����)
	float yPos = 0.f;
	float zPos = 0.f;
	float scale = 1.f;
	int r = 255;
	int g = 255;
	int b = 255;
	int a = 255;
	uint64_t tmpNodeId = 0;    // �������� ����, ������������ ������ �� ����� ��������������
	std::shared_ptr<NodeBase> childNode;
};


struct NodeBase
{
	virtual double GetDensity(float x, float y, float z, bool isHardBrush, float brushCoeff) = 0;  // ���� ��������� � ����� (� ������� ��������� �����)
	virtual void serializeNode(FILE* f, const std::function<bool (NodeBase*)>& isSerializedAlready) = 0;
	void serializeBase(FILE* f);
	void deserializeBase(FILE* f);

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

	void serializeNode(FILE* f, const std::function<bool(NodeBase*)>& isSerializedAlready) override;
	void deserializeNode(FILE* f, MapRef& mapRef);
	double GetDensity(float x, float y, float z, bool isHardBrush, float brushCoeff) override; // ���� ��������� � ����� (� ������� ��������� �����)
	void generate3dCloud(std::vector<float>& dst, int bufSize, float xPos, float yPos, float zPos, float scale, bool isHardBrush, float brushCoeff);
	void serializeAll(const std::string& fileName);
	void deserializeAll(const std::string& fileName);
};

struct NodeLeaf : public NodeBase
{
	double GetDensity(float x, float y, float z, bool isHardBrush, float brushCoeff) override;  // ���� ��������� � ����� (� ������� ��������� �����)
	void serializeNode(FILE* f, const std::function<bool(NodeBase*)>& isSerializedAlready) override;
	void deserializeNode(FILE* f);
};


