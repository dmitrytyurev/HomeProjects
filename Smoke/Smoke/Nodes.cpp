#include "pch.h"
#include <map>
#include "Nodes.h"
#include "utils.h"
//--------------------------------------------------------------------------------------------

double NodeBranch::GetDensity(float x, float y, float z, bool isHardBrush, float brushCoeff)
{
	x += xCenter;  // Переходим из системы координат глифа ноды в систему координат ноды
	y += yCenter;
	z += zCenter;

	if (x < bboxX1 || x > bboxX2 || y < bboxY1 || y > bboxY2 || z < bboxZ1 || z > bboxZ2) {
		return 0.;
	}

	double result = 1.;
	for (const auto& nodeRef: childNodes) {
		float childScale = nodeRef.scale;
		if (nodeRef.childNode->isLeaf()) {
			childScale *= 1.3f;                      // !!! const
		}
		result *= 1. - nodeRef.childNode->GetDensity((x - nodeRef.xPos)/ childScale, (y - nodeRef.yPos) / childScale, (z - nodeRef.zPos) / childScale, isHardBrush, brushCoeff);
	}
	return 1. - result;
}

//--------------------------------------------------------------------------------------------

double NodeLeaf::GetDensity(float x, float y, float z, bool isHardBrush, float brushCoeff)
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
		return (cos(ratio * PI) + 1) * 0.5 * brushCoeff;                                                                            // const !!!
	}
}

//--------------------------------------------------------------------------------------------

void NodeBranch::generate3dCloud(std::vector<float>& dst, int bufSize, float xPos, float yPos, float zPos, float scale, bool isHardBrush, float brushCoeff)
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
		//printf("z: %d of %d\n", z, bufSize);
		for (int y = 0; y < bufSize; ++y) {
			for (int x = 0; x < bufSize; ++x) {
				dst[z*bufSize*bufSize + y * bufSize + x] = (float)GetDensity((x - xPos)/scale, (y - yPos)/scale, (z - zPos)/scale, isHardBrush, brushCoeff);
			}
		}
	}
}

//--------------------------------------------------------------------------------------------

void NodeBranch::serializeAll(const std::string& fileName)
{
	std::vector<NodeBase*> serialized;

	FILE* f = fopen(fileName.c_str(), "wb");
	if (f == nullptr) {
		exit_msg("Error creating file: %s", fileName.c_str());
	}
	serializeNode(f, [&serialized](NodeBase* pNodeBase) {  
		bool isSerialized = std::find(serialized.begin(), serialized.end(), pNodeBase) != serialized.end(); 
		serialized.push_back(pNodeBase);  
		return isSerialized;  
	});
	fclose(f);
}

//--------------------------------------------------------------------------------------------

void NodeBranch::deserializeAll(const std::string& fileName)
{
	MapRef nodeMap;  // Соответствие между id ноды и её shared_ptr

	FILE* f = fopen(fileName.c_str(), "rb");
	if (f == nullptr) {
		exit_msg("Error openinging file: %s", fileName.c_str());
	}
	uint64_t id = 0;
	fread(&id, sizeof(uint64_t), 1, f);
	int type = 0;
	fread(&type, sizeof(int), 1, f);
	deserializeNode(f, nodeMap);
	fclose(f);
}

//--------------------------------------------------------------------------------------------

void NodeBranch::deserializeNode(FILE* f, MapRef& mapRef)
{
	deserializeBase(f);
	int childNodesNum = 0;
	fread(&childNodesNum, sizeof(int), 1, f);
	for (int i=0; i< childNodesNum; ++i) {
		childNodes.push_back(NodeRef());
		childNodes.back().deserialize(f);
	}

	while (true)
	{
		bool isUnfilled = false;
		for (auto& childRef : childNodes) {
			if (!childRef.childNode) {
				if (mapRef.find(childRef.tmpNodeId) == mapRef.end()) {
					isUnfilled = true;
				}
				else {
					childRef.childNode = mapRef[childRef.tmpNodeId];
				}
			}
		}
		if (!isUnfilled) {
			return;
		}
		uint64_t id = 0;
		fread(&id, sizeof(uint64_t), 1, f);
		int type = 0;
		fread(&type, sizeof(int), 1, f);
		if (type == 0) {
			std::shared_ptr<NodeBranch> nb = std::make_shared<NodeBranch>();
			mapRef.insert(std::pair<uint64_t, std::shared_ptr<NodeBase>>(id, nb));
			nb->deserializeNode(f, mapRef);
		}
		else {
			std::shared_ptr<NodeLeaf> nl = std::make_shared<NodeLeaf>();
			mapRef.insert(std::pair<uint64_t, std::shared_ptr<NodeBase>>(id, nl));
			nl->deserializeNode(f);
		}
	}
}

//--------------------------------------------------------------------------------------------

void NodeLeaf::deserializeNode(FILE* f)
{
	deserializeBase(f);
}

//--------------------------------------------------------------------------------------------


void NodeBase::serializeBase(FILE* f)
{
	fwrite(&xCenter, sizeof(float), 1, f);
	fwrite(&yCenter, sizeof(float), 1, f);
	fwrite(&zCenter, sizeof(float), 1, f);
	fwrite(&bboxX1, sizeof(float), 1, f);
	fwrite(&bboxY1, sizeof(float), 1, f);
	fwrite(&bboxZ1, sizeof(float), 1, f);
	fwrite(&bboxX2, sizeof(float), 1, f);
	fwrite(&bboxY2, sizeof(float), 1, f);
	fwrite(&bboxZ2, sizeof(float), 1, f);
}

//--------------------------------------------------------------------------------------------

void NodeBase::deserializeBase(FILE* f)
{
	fread(&xCenter, sizeof(float), 1, f);
	fread(&yCenter, sizeof(float), 1, f);
	fread(&zCenter, sizeof(float), 1, f);
	fread(&bboxX1, sizeof(float), 1, f);
	fread(&bboxY1, sizeof(float), 1, f);
	fread(&bboxZ1, sizeof(float), 1, f);
	fread(&bboxX2, sizeof(float), 1, f);
	fread(&bboxY2, sizeof(float), 1, f);
	fread(&bboxZ2, sizeof(float), 1, f);
}

//--------------------------------------------------------------------------------------------

void NodeRef::serialize(FILE* f) const
{
	fwrite(&xPos, sizeof(float), 1, f);
	fwrite(&yPos, sizeof(float), 1, f);
	fwrite(&zPos, sizeof(float), 1, f);
	fwrite(&scale, sizeof(float), 1, f);
	fwrite(&r, sizeof(int), 1, f);
	fwrite(&g, sizeof(int), 1, f);
	fwrite(&b, sizeof(int), 1, f);
	fwrite(&a, sizeof(int), 1, f);
	uint64_t nodeId = (uint64_t)childNode.get();
	fwrite(&nodeId, sizeof(uint64_t), 1, f);
}
//--------------------------------------------------------------------------------------------

void NodeRef::deserialize(FILE* f)
{
	fread(&xPos, sizeof(float), 1, f);
	fread(&yPos, sizeof(float), 1, f);
	fread(&zPos, sizeof(float), 1, f);
	fread(&scale, sizeof(float), 1, f);
	fread(&r, sizeof(int), 1, f);
	fread(&g, sizeof(int), 1, f);
	fread(&b, sizeof(int), 1, f);
	fread(&a, sizeof(int), 1, f);
	fread(&tmpNodeId, sizeof(uint64_t), 1, f);
}

//--------------------------------------------------------------------------------------------

void NodeBranch::serializeNode(FILE* f, const std::function<bool(NodeBase*)>& isSerializedAlready)
{
	if (isSerializedAlready(this)) {
		return;
	}
	uint64_t nodeId = (uint64_t)this;
	fwrite(&nodeId, sizeof(uint64_t), 1, f);
	int type = 0;
	fwrite(&type, sizeof(int), 1, f);
	serializeBase(f);
	int childNodesNum = (int)childNodes.size();
	fwrite(&childNodesNum, sizeof(int), 1, f);
	for (const auto& node : childNodes) {
		node.serialize(f);
	}
	for (const auto& node : childNodes) {
		node.childNode->serializeNode(f, isSerializedAlready);
	}
}

//--------------------------------------------------------------------------------------------

void NodeLeaf::serializeNode(FILE* f, const std::function<bool(NodeBase*)>& isSerializedAlready)
{
	if (isSerializedAlready(this)) {
		return;
	}
	uint64_t nodeId = (uint64_t)this;
	fwrite(&nodeId, sizeof(uint64_t), 1, f);
	int type = 1;
	fwrite(&type, sizeof(int), 1, f);
	serializeBase(f);
}

