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
	struct HistoryFile
	{
		std::string name;                // Имя файла истории текущей базы (пустая строка, если файл истории ещё не создавался - с момента чтения/записи основного файла не было событий)
		double timeToFlush = 0;          // Время в секундах до флаша буфера истории на диск
		uint32_t savedFileSize = 0;      // Объём файла истории, который уже записан на диск
		SerializationBuffer buffer;
	};

	DbSerializer(TextsDatabase* pDataBase);
	void SetPath(const std::string& path);
	void Update(double dt);

	void SaveDatabase();  // Имя файла базы конструирует из имени базы
	bool LoadDatabaseAndHistory(); // Имена файлов базы и истории конструирует из имени базы, выбирает самые свежие файлы

	void HistoryFlush();
	SerializationBuffer& GetHistoryBuffer();
	uint32_t GetCurrentPosInHistoryFile();  // Возвращает текущее смещение от начала файла истории (включая содержимое буфера)

private:
	std::string FindFreshBaseFileName(uint32_t& resultTimestamp);
	std::string FindHistoryFileName(uint32_t tsBaseFile);
	void LoadDatabaseInner(const std::string& fullFileName);
	void LoadHistoryInner(const std::string& fullFileName);

	std::string _path;            // Путь, где хранятся файлы базы и файлы истории
	HistoryFile _historyFile;     // Информация о файле истории
	TextsDatabase* _pDataBase = nullptr;
};
