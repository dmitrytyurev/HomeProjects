#pragma once
#include "TextsBaseClasses.h"

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
	bool IsEmpty() { return buffer.size()-1 == offset; }

	uint32_t offset = 0;
	std::vector<uint8_t> buffer;
};

//===============================================================================
// Запись и чтение базы текстов на диск (как целиком, так и история)
//===============================================================================

class TextsDatabase;
class Folder;

class DbSerializer
{
public:
	enum ActionType
	{
		ActionCreateFolder = 0,         // Создание каталога для текстов
	};

	struct HistoryFile
	{
		std::string name;                // Имя файла истории текущей базы (пустая строка, если файл истории ещё не создавался - с момента чтения/записи основного файла не было событий)
		double timeToFlush = 0;          // Время в секундах до флаша буфера истории на диск
		SerializationBuffer buffer;
	};

	DbSerializer(TextsDatabase* pDataBase);
	void SetPath(const std::string& path);
	void Update(double dt);

	void SaveDatabase();  // Имя файла базы конструирует из имени базы
	void LoadDatabaseAndHistory(); // Имена файлов базы и истории конструирует из имени базы, выбирает самые свежие файлы

	void HistoryFlush();
	SerializationBuffer& GetSerialBuffer();
	static void PushCommonHeader(SerializationBuffer& buffer, uint32_t timestamp, const std::string& loginOfLastModifier, uint8_t actionType);

private:
	std::string FindFreshBaseFileName(uint32_t& resultTimestamp);
	std::string FindHistoryFileName(uint32_t tsBaseFile);
	void LoadDatabaseInner(const std::string& fullFileName);
	void LoadHistoryInner(const std::string& fullFileName);

	std::string _path;            // Путь, где хранятся файлы базы и файлы истории
	HistoryFile _historyFile;     // Информация о файле истории
	TextsDatabase* _pDataBase = nullptr;
};

//===============================================================================
//
//===============================================================================

class PeriodUpdater
{
public:
	void SetPeriod(double period);
	bool IsTimeToProceed(double dt);

private:
	double _timeToProcess = 0;
	double _period = 1;
};

//===============================================================================
// Обрабатывает входящую очередь сообщений от клиентов, модифицирует базу под действием этих сообщений, 
// записывает на диск историю, добавляет сообщения в очередь на отсылку (для синхронизации всем клиентам -
// в том числе и тем, от кого пришли сообщения)
// Также обрабатывает очередь подключений и отключений. На основании этого создаёт/удаляет объекты 
// SConnectedClient. Для новых объектов накидывает в очередь сообщения о стартовой синхронизации.
//===============================================================================

class STextsToolApp;

class SClientMessagesMgr
{
public:
	SClientMessagesMgr(STextsToolApp* app);
	void Update(double dt);

	STextsToolApp* _app;

private:
	TextsDatabase::Ptr GetDbPtrByDbName(const std::string& dbName);
};

//===============================================================================
//
//===============================================================================

class MsgInQueue
{
public:
	using Ptr = std::unique_ptr<MsgInQueue>;

	uint8_t actionType; // Одно из значений DbSerializer::ActionType
	DeserializationBuffer _msgData; // Сериализованные данные события полученные от клиента и готовые к отсылке на клиент
};

//===============================================================================
//
//===============================================================================

class MutexLock
{
public:
	MutexLock(std::mutex& mutex) { pMutex = &mutex; mutex.lock(); }
	~MutexLock() { pMutex->unlock();  }
private:
	std::mutex* pMutex;
};

//===============================================================================
//
//===============================================================================

class HttpPacket
{
public:
	using Ptr = std::unique_ptr<HttpPacket>;

	uint32_t packetIndex; // Порядковый номер пакета
	std::vector<uint8_t> packetData;
};

//===============================================================================
//
//===============================================================================

class MTQueue
{
public:
	std::vector<HttpPacket::Ptr> queue;
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class SConnectedClientLow
{
	std::string login;
	std::string password;
	uint32_t timestampLastRequest;  // Когда от этого клиента приходил последний запрос
	MTQueue packetsQueueOut;
	MTQueue packetsQueueIn;
	uint32_t lastRecievedPacketN;
};

//===============================================================================
//
//===============================================================================

class SConnectedClient
{
public:
	using Ptr = std::shared_ptr<SConnectedClient>;

	std::string login;
	std::string _dbName;  // Имя база, с которой сейчас работает клиент
	std::vector<MsgInQueue::Ptr> _msgsQueueOut; // Очередь сообщений, которые нужно отослать клиенту
	std::vector<MsgInQueue::Ptr> _msgsQueueIn;  // Очередь пришедших от клиента сообщений
};

//===============================================================================
//
//===============================================================================

class STextsToolApp
{
public:
	STextsToolApp();
	void Update(double dt);

	std::vector<TextsDatabase::Ptr> _dbs;
	std::vector<SConnectedClient::Ptr> _clients;

	SClientMessagesMgr _messagesMgr;

	//	SHttpManager httpManager;
	//	SMessagesRepaker messagesRepaker;
};




