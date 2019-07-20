#pragma once
#include <string>
#include <vector>

//===============================================================================
// 
//===============================================================================

class SerializationBuffer
{
public:
	void Push(uint8_t v);
	void Push(uint16_t v);
	void Push(uint32_t v);
	void Push(const std::string& v);
	void PushStringWithoutZero(const std::string& v);
	void PushBytes(const void* bytes, int size);

	std::vector<uint8_t> buffer;
};

//===============================================================================
// ������ � ������ ���� ������� �� ���� (��� �������, ��� � �������)
//===============================================================================

class TextsDatabase;

class DbSerializer
{
public:
	DbSerializer(TextsDatabase* pDataBase);
	void Update(float dt); // ��� ������� ��� �������� ����� ����� 1 ��� ����� ������

	void SaveDatabase();  // ��� ����� ���� ������������ �� ����� ����
	void LoadDatabaseAndHistory(); // ����� ������ ���� � ������� ������������ �� ����� ����, �������� ����� ������ �����

//	void SaveCreateFolder(const Folder& folder);
//	void SaveRenameFolder(const Folder& folder);

	TextsDatabase* _pDataBase;
};

//===============================================================================
// 
//===============================================================================


class ISerializable
{
public:
	virtual void serialize(SerializationBuffer& buffer) const = 0;
//	virtual void deserialize(DeserializationBuffer& buffer) = 0;
};

//===============================================================================
// �������� �������� (������� � ���� �������)
//===============================================================================

class AttributeProperty: public ISerializable
{
public:
	enum DataType
	{
		Translation_t = 0,  // ����� ������ �� �������������� ������
		CommonText_t = 1,   // ����� ������ ���������� (�� �������)
		Checkbox_t = 2,     // �������
	};

	virtual void serialize(SerializationBuffer& buffer) const;

	uint8_t id;           // ID ��������
	std::string name;     // ��� ��������
	uint8_t type;         // �������� ������ �� ����� DataType
	uint32_t param1;      // ��������, ��������� �� ���� (��� Translation_t - ��� id �����)
	uint32_t param2;      // ��������, ��������� �� ���� (�����)
	uint8_t visiblePosition;  // ���������� ������� �������� � ������� (���� ���� �����)
	bool isVisible;       // ��������� ��������
	uint32_t timestampCreated;  // ����� ��������
};

//===============================================================================
// ������ �������� � ������
//===============================================================================

class AttributeInText: public ISerializable
{
public:
	virtual void serialize(SerializationBuffer& buffer) const;

	std::string text;   // ��������� ������ ��������, ���� ��� �����
	uint8_t flagState;  // ��������� ������, ���� ��� ������;
	uint8_t id;         // ID ��������, �� �������� ����� ������ ��� ���
	uint8_t type;       // �������� ������ �� ����� AttributeProperty::DataType. !!!����� ����� ��� �������� �������! � ����� �� �����, ��������� ��� ����� ���������� �� ID ��������
};


//===============================================================================
// ����� �� ����� ���������� � ������� ���������� (������� � �������)
//===============================================================================

class TextTranslated: public ISerializable
{
public:
	using Ptr = std::shared_ptr<TextTranslated>;

	virtual void serialize(SerializationBuffer& buffer) const;

	std::string id;                   // ID ������
	uint32_t timestampCreated;        // ����� ��������
	uint32_t timestampModified;   // ����� ���������� ���������
	std::string loginOfLastModifier;  // ����� ����, ��� ����� ��������� ���
	uint32_t offsLastModified;        // �������� � ����� ������� �� ������ � ��������� ���������
	std::string baseText;             // ����� �� ������� ����� (�������)
	std::vector<AttributeInText> attributes; // �������� ������ � �� �������
};


//===============================================================================
// ������� � �������� � ���������� ���������� � ���� �������
//===============================================================================

class Folder: public ISerializable
{
public:
	virtual void serialize(SerializationBuffer& buffer) const;

	uint32_t id;                 // ID �����
	uint32_t timestampCreated;   // ����� ��������
	uint32_t timestampModified;  // ����� ��������� ������ ����� ��� � �������
	std::string name;            // ��� �����
	uint32_t parentId;           // ID ������������ �����
	std::vector<TextTranslated::Ptr> texts;  // ������ ������� ��������������� � �����
};

//===============================================================================
// ���� �������
//===============================================================================

class TextsDatabase
{
public:

	TextsDatabase(const std::string dbName); // ��������� � ������ ���� �� ������ ������
	void Update(float dt);

	std::string _dbName;           // ��� ���� ������ �������
	std::vector<AttributeProperty> _attributeProps; // �������� ��������� (�������) �������
	std::vector<Folder> _folders;  // �����. ����������� ��������� ����� ������ �� ID ��������

	DbSerializer _dbSerializer; // ������/������ ���� ������� �� ����
};
