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
	};

	struct HistoryFile
	{
		std::string name;                // ��� ����� ������� ������� ���� (������ ������, ���� ���� ������� ��� �� ���������� - � ������� ������/������ ��������� ����� �� ���� �������)
		double timeToFlush = 0;          // ����� � �������� �� ����� ������ ������� �� ����
		SerializationBuffer buffer;
	};

	DbSerializer(TextsDatabase* pDataBase);
	void SetPath(const std::string& path);
	void Update(double dt);

	void SaveDatabase();  // ��� ����� ���� ������������ �� ����� ����
	void LoadDatabaseAndHistory(); // ����� ������ ���� � ������� ������������ �� ����� ����, �������� ����� ������ �����

	void HistoryFlush();
	SerializationBuffer& GetHistoryBuffer();

private:
	std::string FindFreshBaseFileName(uint32_t& resultTimestamp);
	std::string FindHistoryFileName(uint32_t tsBaseFile);
	void LoadDatabaseInner(const std::string& fullFileName);
	void LoadHistoryInner(const std::string& fullFileName);

	std::string _path;            // ����, ��� �������� ����� ���� � ����� �������
	HistoryFile _historyFile;     // ���������� � ����� �������
	TextsDatabase* _pDataBase = nullptr;
};
