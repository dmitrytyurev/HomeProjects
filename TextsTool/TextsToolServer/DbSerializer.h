#pragma once

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"


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
		ActionCreateFolder = 0,         // �������� �������� ��� �������
		ActionDeleteFolder = 1,         // �������� �������� (������ �� ����� ������� � ��������� ���������)
		ActionChangeFolderParent = 2,   // ��������� ������������� ��������
		ActionRenameFolder = 3,         // �������������� ��������
		ActionCreateAttribute = 4,      // �������� �������� �������
		ActionDeleteAttribute = 5,      // �������� �������� �������
		ActionRenameAttribute = 6,      // �������������� �������� �������
		ActionChangeAttributeVis = 7,   // ��������� ������������� ����������� ������ �������� � ������� ������� ��� ��������� ��������
		ActionCreateText = 8,           // �������� ������
		ActionDeleteText = 9,           // �������� ������
	};

	struct HistoryFile
	{
		std::string name;                // ��� ����� ������� ������� ���� (������ ������, ���� ���� ������� ��� �� ���������� - � ������� ������/������ ��������� ����� �� ���� �������)
		double timeToFlush = 0;          // ����� � �������� �� ����� ������ ������� �� ����
		uint32_t savedFileSize = 0;      // ����� ����� �������, ������� ��� ������� �� ����
		SerializationBuffer buffer;
	};

	DbSerializer(TextsDatabase* pDataBase);
	void SetPath(const std::string& path);
	void Update(double dt);

	void SaveDatabase();  // ��� ����� ���� ������������ �� ����� ����
	void LoadDatabaseAndHistory(); // ����� ������ ���� � ������� ������������ �� ����� ����, �������� ����� ������ �����

	void HistoryFlush();
	SerializationBuffer& GetHistoryBuffer();
	uint32_t GetCurrentPosInHistoryFile();  // ���������� ������� �������� �� ������ ����� ������� (������� ���������� ������)

private:
	std::string FindFreshBaseFileName(uint32_t& resultTimestamp);
	std::string FindHistoryFileName(uint32_t tsBaseFile);
	void LoadDatabaseInner(const std::string& fullFileName);
	void LoadHistoryInner(const std::string& fullFileName);

	std::string _path;            // ����, ��� �������� ����� ���� � ����� �������
	HistoryFile _historyFile;     // ���������� � ����� �������
	TextsDatabase* _pDataBase = nullptr;
};
