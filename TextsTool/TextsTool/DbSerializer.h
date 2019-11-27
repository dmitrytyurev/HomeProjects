#pragma once

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"


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
	void Update(double dt);

	void SaveDatabase();  //   Записывает всю базу в файл. Имя файла базы конструирует из имени базы
	bool LoadDatabase();  // Читает базу из полного файла 

private:
	std::string FindFreshBaseFileName(uint32_t& resultTimestamp);
	void LoadDatabaseInner(const std::string& fullFileName);

	std::string _path;            // Путь, где хранятся файлы базы и файлы истории
	TextsDatabase* _pDataBase = nullptr;
};
