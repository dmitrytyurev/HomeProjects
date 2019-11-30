#pragma once

#include "TextsBaseClasses.h"
#include "../SharedSrc/SerializationBuffer.h"
#include "../SharedSrc/DeserializationBuffer.h"


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

	void SaveDatabase();
	void LoadDatabase();

private:

	std::string _path;            // ����, ��� �������� ����� ���� � ����� �������
	TextsDatabase* _pDataBase = nullptr;
};
