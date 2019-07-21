#pragma once
#include <string>
#include <vector>
#include <fstream>

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

	template <typename T>
	void PushStringWithoutZero(const std::string& v);


	void PushBytes(const void* bytes, int size);

	std::vector<uint8_t> buffer;
};

//===============================================================================
// 
//===============================================================================

class DeserializationBuffer
{
public:
	template <typename T>
	T GetUint();
	template <typename T>
	void GetString(std::string& result);

	uint32_t offset = 0;
	std::vector<uint8_t> buffer;
};

//===============================================================================
// ������ � ������ ���� ������� �� ���� (��� �������, ��� � �������)
//===============================================================================

class TextsDatabase;
class Folder;

class DbSerializer
{
public:
	enum ActionType
	{
		ActionCreateFolder = 0,  // ������ ������� ��� �������
	};

	struct HistoryFile
	{
		std::string name;                // ��� ����� ������� ������� ���� (������ ������, ���� ���� ������� ��� �� ���������� - � ������� ������/������ ��������� ����� �� ���� �������)
		std::ofstream stream;            // �������� ��� �������� ����� ����� �������. ���� ����������� ����� ��������� ������ ����� ��������� ������.
		uint32_t lastWriteTimestamp = 0; // ����� ��������� ������ � ���� �������
		SerializationBuffer serializationBuffer;
	};

	DbSerializer(TextsDatabase* pDataBase);
	void SetPath(const std::string& path);
	void Update(float dt); // ��� ������� ��� �������� ����� ����� 1 ��� ����� ������

	void SaveDatabase();  // ��� ����� ���� ������������ �� ����� ����
	void LoadDatabaseAndHistory(); // ����� ������ ���� � ������� ������������ �� ����� ����, �������� ����� ������ �����

	void SaveCreateFolder(const Folder& folder, const std::string& loginOfLastModifier);

private:
	std::string GetFreshBaseFileName();
	void SaveCommonHeader(uint32_t timestamp, const std::string& loginOfLastModifier, uint8_t actionType);

	std::string _path;            // ����, ��� �������� ����� ���� � ����� �������
	HistoryFile _historyFile;     // ���������� � ����� �������
	TextsDatabase* _pDataBase = nullptr;
};

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
	AttributeProperty(DeserializationBuffer& buffer);
	void serialize(SerializationBuffer& buffer) const;

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
	AttributeInText(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void serialize(SerializationBuffer& buffer) const;

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
	TextTranslated(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void serialize(SerializationBuffer& buffer) const;

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
	Folder(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void serialize(SerializationBuffer& buffer) const;

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

	TextsDatabase(const std::string dbName); // ��������� � ������ ���� �� ������ ������
	void Update(float dt);

	std::string _dbName;           // ��� ���� ������ �������
	std::vector<AttributeProperty> _attributeProps; // �������� ��������� (�������) �������
	std::vector<Folder> _folders;  // �����. ����������� ��������� ����� ������ �� ID ��������

	DbSerializer _dbSerializer; // ������/������ ���� ������� �� ����
};
