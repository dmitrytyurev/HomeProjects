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
	void CreateFromBase(DeserializationBuffer& buffer);
	void SaveToBase(SerializationBuffer& buffer) const;

	void CreateFromHistory(DeserializationBuffer& buffer, uint32_t ts);  // �������� ������� �� ����� �������
	void SaveToHistory(TextsDatabasePtr db, const std::string& loginOfLastModifier) const;

	void CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId);   // �������� ������� �� ��������� �� ������� 
	SerializationBufferPtr SaveToPacket(const std::string& loginOfModifier) const;      // ������ ������� � ����� ��� �������� ���� ��������, ���������� � ���� �����

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
	void CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void SaveToBase(SerializationBuffer& buffer) const;

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
	using Ptr = std::shared_ptr<TextTranslated>;

	TextTranslated() {}
	void CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void SaveToBase(SerializationBuffer& buffer) const;

	void SaveToHistory(TextsDatabasePtr db, uint32_t folderId);

	SerializationBufferPtr SaveToPacket(uint32_t folderId, const std::string& loginOfModifier) const;      // ������ ������� � ����� ��� �������� ���� ��������, ���������� � ���� �����


	std::string id;                   // ID ������
	uint32_t timestampCreated = 0;    // ����� ��������
	uint32_t timestampModified = 0;   // ����� ���������� ���������
	std::string loginOfLastModifier;  // ����� ����, ��� ����� ��������� ���
	uint32_t offsLastModified = 0;    // �������� � ����� ������� �� ������ � ��������� ���������
	std::string baseText;             // ����� �� ������� ����� (�������)
	std::vector<AttributeInText> attributes; // �������� ������ � �� �������
};


//===============================================================================
// ������� � �������� � ���������� ���������� � ���� �������
//===============================================================================

class Folder
{
public:
	Folder() {}
	void CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);  // �������� ������� �� ������� ����� ����
	void SaveToBase(SerializationBuffer& buffer) const;      // ������ ������� � ������ ���� ����

	void CreateFromHistory(DeserializationBuffer& buffer, uint32_t ts);  // �������� ������� �� ����� �������
	void SaveToHistory(TextsDatabasePtr db, const std::string& loginOfLastModifier) const;

	void CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId);   // �������� ������� �� ��������� �� ������� 
	SerializationBufferPtr SaveToPacket(const std::string& loginOfModifier) const;      // ������ ������� � ����� ��� �������� ���� ��������, ���������� � ���� �����

	uint32_t id = 0;                 // ID �����
	uint32_t timestampCreated = 0;   // ����� ��������
	uint32_t timestampModified = 0;  // ����� ��������� ������ ����� ��� � �������
	std::string name;                // ��� �����
	uint32_t parentId = 0;           // ID ������������ �����
	std::vector<TextTranslated::Ptr> texts;  // ������ ������� ��������������� � �����
};

//===============================================================================
// ���� �������
//===============================================================================

class TextsDatabase
{
public:
	void CreateDatabase(const std::string path, const std::string dbName); // ������ ���� � ������, ������ ������ ���� �� �����
	void LoadDatabase(const std::string path, const std::string dbName); // ������ ���� � ������ �� ����� ���� � ����� �������
	void Update(double dt);
	SerializationBuffer& GetHistoryBuffer();
	uint32_t GetCurrentPosInHistoryFile();

	std::string _dbName;           // ��� ���� ������ �������
	uint8_t _newAttributeId = 0;   // ����� ������������ ������ ����� �������, ���� ���� �����. ���� �����������.
	std::vector<AttributeProperty> _attributeProps; // �������� ��������� (�������) �������
	uint32_t _newFolderId = 0;     // ����� ������������ ������ ����� �����, ���� ���� �����. ���� �����������.
	std::vector<Folder> _folders;  // �����. ����������� ��������� ����� ������ �� ID ��������

	DbSerializerPtr _dbSerializer; // ������/������ ���� ������� �� ����
};

