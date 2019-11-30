#pragma once

#include "TextsBaseClasses.h"
#include "../SharedSrc/SerializationBuffer.h"
#include "../SharedSrc/DeserializationBuffer.h"


//===============================================================================
// Запись и чтение базы текстов на диск (как целиком, так и история)
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

	std::string _path;            // Путь, где хранятся файлы базы и файлы истории
	TextsDatabase* _pDataBase = nullptr;
};
