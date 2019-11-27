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
	DbSerializer(TextsDatabase* pDataBase);
	void SetPath(const std::string& path);
	void Update(double dt);

	void SaveDatabase();  //   ���������� ��� ���� � ����. ��� ����� ���� ������������ �� ����� ����
	bool LoadDatabase();  // ������ ���� �� ������� ����� 

private:
	std::string FindFreshBaseFileName(uint32_t& resultTimestamp);
	void LoadDatabaseInner(const std::string& fullFileName);

	std::string _path;            // ����, ��� �������� ����� ���� � ����� �������
	TextsDatabase* _pDataBase = nullptr;
};
