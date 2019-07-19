#pragma once
#include <string>
#include <vector>

//===============================================================================
// �������� �������� (������� � ���� �������)
//===============================================================================

class AttributeProperty //: public ISerializable
{
public:

	enum DataType
	{
		Translation_t = 0,  // ����� ������ �� �������������� ������
		CommonText_t = 1,   // ����� ������ ���������� (�� �������)
		Checkbox_t = 2,     // �������
	};

	uint8_t id;           // ID ��������
	std::string name;     // ��� ��������
	uint8_t type;         // �������� ������ �� ����� DataType
	uint32_t param1;      // ��������, ��������� �� ���� (��� Translation_t - ��� id �����)
	uint32_t param2;      // ��������, ��������� �� ���� (�����)
	int visiblePosition;  // ���������� ������� �������� � ������� (���� ���� �����)
	bool isVisible;       // ��������� ��������
	uint32_t timestampCreated;  // ����� ��������
};

//===============================================================================
// ������ �������� � ������
//===============================================================================

class AttributeInText //: public ISerializable
{
public:

	std::string text;   // ��������� ������ ��������, ���� ��� �����
	uint8_t flagState;  // ��������� ������, ���� ��� ������;
	uint8_t id;         // ID ��������, �� �������� ����� ������ ��� ���
};


//===============================================================================
// ����� �� ����� ���������� � ������� ���������� (������� � �������)
//===============================================================================

class TextTranslated //: public ISerializable
{
public:
	using Ptr = std::shared_ptr<TextTranslated>;

	std::string id;                   // ID ������
	uint32_t timestampCreated;        // ����� ��������
	uint32_t timestampLastModified;   // ����� ���������� ���������
	std::string loginOfLastModifier;  // ����� ����, ��� ����� ��������� ���
	uint32_t offsLastModified;        // �������� � ����� ������� �� ������ � ��������� ���������
	std::string baseText;             // ����� �� ������� ����� (�������)
	std::vector<AttributeInText> attributes; // �������� ������ � �� �������
};


//===============================================================================
// ������� � �������� � ���������� ���������� � ���� �������
//===============================================================================

class Folder // : public ISerializable
{
public:

	uint32_t id;                 // ID �����
	uint32_t timestampCreated;   // ����� ��������
	uint32_t timestampModified;  // ����� ��������� ������ ����� ��� � �������
	std::string name;            // ��� �����
	uint32_t id;                 // ID ������������ �����
	std::vector<TextTranslated::Ptr> texts;  // ������ ������� ��������������� � �����
};

//===============================================================================
// ���� �������
//===============================================================================

class TextsDatabase
{
public:

	TextsDatabase(const std::string DbName); // ��������� � ������ ���� �� ������ ������
	void Update(float dt);

	DbSerializer dbSerializer;
	std::string dbName;           // ��� ���� ������ �������
	std::vector<AttributeProperty> attributeProps; // �������� ��������� (�������) �������
	std::vector<Folder> folders;  // �����. ����������� ��������� ����� ������ �� ID ��������
};
