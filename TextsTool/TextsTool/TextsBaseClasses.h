#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

class DeserializationBuffer;
class SerializationBuffer;
class DbSerializer;
class TextsDatabase;

using DbSerializerPtr = std::unique_ptr<DbSerializer>;
using SerializationBufferPtr = std::shared_ptr<SerializationBuffer>;
using TextsDatabasePtr = std::shared_ptr<TextsDatabase>;


//===============================================================================
// �������� �������� (������� � ���� �������)
//===============================================================================

class AttributeProperty
{
public:
	enum DataType
	{
		Translation_t = 0,  // ����� ������ �� �������������� ������
		CommonText_t = 1,   // ����� ������ ���������� (�� �������)
		Checkbox_t = 2,     // �������
	};

	AttributeProperty() {}
	void LoadFullDump(DeserializationBuffer& buffer);
	void SaveFullDump(SerializationBuffer& buffer) const;

	void CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId);   // �������� ������� �� ��������� �� ������� 
	void Log(const std::string& prefix);

	uint8_t id = 0;           // ID ��������
	std::string name;         // ��� ��������
	uint8_t type = 0;         // �������� ������ �� ����� DataType
	uint32_t param1 = 0;      // ��������, ��������� �� ���� (��� Translation_t - ��� id �����)
	uint32_t param2 = 0;      // ��������, ��������� �� ���� (�����)
	uint8_t visiblePosition = 0;    // ���������� ������� �������� � ������� (���� ���� �����)
	bool isVisible = false;         // ��������� ��������
	uint32_t timestampCreated = 0;  // ����� ��������
};

//===============================================================================
// ������ �������� � ������
//===============================================================================

class AttributeInText
{
public:
	AttributeInText() {}
	void LoadFullDump(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void SaveFullDump(SerializationBuffer& buffer) const;
	void Log(const std::string& prefix);

	std::string text;       // ��������� ������ ��������, ���� ��� �����
	uint8_t flagState = 0;  // ��������� ������, ���� ��� ������;
	uint8_t id = 0;         // ID ��������, �� �������� ����� ������ ��� ���
	uint8_t type = 0;       // �������� ������ �� ����� AttributeProperty::DataType. !!!����� ����� ��� �������� �������! � ����� �� �����, ��������� ��� ����� ���������� �� ID ��������
};


//===============================================================================
// ����� �� ����� ���������� � ������� ���������� (������� � �������)
//===============================================================================

class TextTranslated
{
public:
	TextTranslated() {}
	void LoadFullDump(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void SaveFullDump(SerializationBuffer& buffer) const;
	void Log(const std::string& prefix);

	std::string id;                   // ID ������
	uint32_t timestampCreated = 0;    // ����� ��������
	uint32_t timestampModified = 0;   // ����� ���������� ���������
	std::string loginOfLastModifier;  // ����� ����, ��� ����� ��������� ���
	uint32_t offsLastModified = 0;    // �������� � ����� ������� �� ������ � ��������� ���������
	std::string baseText;             // ����� �� ������� ����� (�������)
	std::vector<AttributeInText> attributes; // �������� ������ � �� �������
};

using TextTranslatedPtr = std::shared_ptr<TextTranslated>;

//===============================================================================
// ������� � �������� � ���������� ���������� � ���� �������
//===============================================================================

class Folder
{
public:
	Folder() {}
	void LoadFullDump(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);  // �������� ������� � ������� �����
	void SaveFullDump(SerializationBuffer& buffer) const;      // ������ ������� ����� �������

	void CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId);   // �������� ������� �� ��������� �� ������� 
	void Log(const std::string& prefix);

	uint32_t id = 0;                 // ID �����
	uint32_t timestampCreated = 0;   // ����� ��������
	uint32_t timestampModified = 0;  // ����� ��������� ������ ����� ��� � �������
	std::string name;                // ��� �����
	uint32_t parentId = 0;           // ID ������������ �����
	std::vector<TextTranslatedPtr> texts;  // ������ ������� ��������������� � �����
};

//===============================================================================
// ���� �������
//===============================================================================

class TextsDatabase
{
public:
	TextsDatabase(const std::string path, const std::string dbName);
	void LogDatabase();

	std::string _dbName;           // ��� ���� ������ �������
	std::vector<AttributeProperty> _attributeProps; // �������� ��������� (�������) �������
	std::vector<Folder> _folders;  // �����. ����������� ��������� ����� ������ �� ID ��������

	DbSerializerPtr _dbSerializer; // ������/������ ���� ������� �� ����
};

