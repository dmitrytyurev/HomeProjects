#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"

class TextsDatabase;
class Folder;

//===============================================================================
//
//===============================================================================

enum EventType
{
	EventRequestSync = 0,          // Запрос синхронизации при подключении нового клиента
	EventCreateFolder = 1,         // Создание каталога для текстов
	EventDeleteFolder = 2,         // Удаление каталога (должен не иметь текстов и вложенных каталогов)
	EventChangeFolderParent = 3,   // Изменение родительского каталога
	EventRenameFolder = 4,         // Переименование каталога
	EventCreateAttribute = 5,      // Создание атрибута таблицы
	EventDeleteAttribute = 6,      // Удаление атрибута таблицы
	EventRenameAttribute = 7,      // Переименование атрибута таблицы
	EventChangeAttributeVis = 8,   // Изменение отображаемого порядкового номера атрибута в таблице текстов или видимость атрибута
	EventCreateText = 9,           // Создание текста
	EventDeleteText = 10,           // Удаление текста
	EventMoveTextToFolder = 11,    // Текст переместился в другую папку
	EventChangeBaseText = 12,      // Изменился основной текст
	EventAddAttributeToText = 13,  // В текст добавился атрибут
	EventDelAttributeFromText = 14,  // Удалился атрибут из текста
	EventChangeAttributeInText = 15, // Изменилось значение атрибута в тексте
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
	void AddPacketToClients(SerializationBufferPtr& bufPtr, const std::string& srcDbName);
	SerializationBufferPtr MakeSyncMessage(DeserializationBuffer& buf, TextsDatabase& db);
	void ConnectClient();    // Вызывается из HttpMgr::Update при разборе очереди подключений/отключений. Создаёт SConnectedClient.
	void DisconnectClient(); // Вызывается из HttpMgr::Update при разборе очереди подключений/отключений. Удаляет SConnectedClient.

	static bool ModifyDbRenameFolder(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts);
	static bool ModifyDbDeleteFolder(DeserializationBuffer& buf, TextsDatabase& db);
	static bool ModifyDbChangeFolderParent(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts);
	static bool ModifyDbDeleteAttribute(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts);
	static bool ModifyDbRenameAttribute(DeserializationBuffer& buf, TextsDatabase& db);
	static bool ModifyDbChangeAttributeVis(DeserializationBuffer& buf, TextsDatabase& db);
	static bool ModifyDbDeleteText(DeserializationBuffer& buf, TextsDatabase& db, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbMoveTextToFolder(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbChangeBaseText(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbAddAttributeToText(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbDelAttributeFromText(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbChangeAttributeInText(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	
	STextsToolApp* _app = nullptr;

private:
	static bool IfKeyALess(const uint8_t* p1, int size1, const uint8_t* p2, int size2);
	void MakeKey(uint32_t tsModified, const std::string& textId, std::vector<uint8_t>& result);
	void SaveToHistory(TextsDatabasePtr db, const std::string& login, uint8_t ts, const DeserializationBuffer& buf);
	void SendToClients(const std::string& dbName, uint8_t ts, const DeserializationBuffer& buf, const std::string& loginOfLastModifier);
	TextsDatabasePtr GetDbPtrByDbName(const std::string& dbName);
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
	std::mutex* pMutex = nullptr;
};

//===============================================================================
//
//===============================================================================

class ClientFolder
{
public:
	struct Interval
	{
		Interval(uint32_t textsNum, uint64_t keysHash) : textsInIntervalNum(textsNum), hashOfKeys(keysHash) {}
		uint32_t textsInIntervalNum = 0;  // Количество текстов в интервале
		uint64_t hashOfKeys = 0;          // Хэш ключей текстов интервала
	};

	ClientFolder() {}
	ClientFolder(DeserializationBuffer& buf)
	{
		id = buf.GetUint<uint32_t>();
		tsModified = buf.GetUint<uint32_t>();
		uint32_t keysNum = buf.GetUint<uint32_t>();
		for (uint32_t keyIdx = 0; keyIdx < keysNum; ++keyIdx) {
			keys.emplace_back(buf.GetVector<uint8_t>());
		}
		for (uint32_t intervalIdx = 0; intervalIdx < keysNum + 1; ++intervalIdx) {
			uint32_t textsInIntervalNum = buf.GetUint<uint32_t>();    // Число текстов в интервале
			uint64_t hashOfKeys = buf.GetUint<uint64_t>();            // CRC64 ключей входящих в группу текстов
			intervals.emplace_back(textsInIntervalNum, hashOfKeys);
		}
	}

	uint32_t id = 0;   // Id папки
	uint32_t tsModified = 0; // Ts изменения папки на клиенте
	std::vector<std::vector<uint8_t>> keys;      // Ключи, разбивающие тексты на интервалы. Ключ - бинарная строка: 4 байта ts + текстовый id-текста 
	std::vector<Interval> intervals;     // Параметры интервалов, задаваемых ключами (интервалов на один больше чем ключей)
};

//===============================================================================
//
//===============================================================================

class HttpPacket
{
public:
	using Ptr = std::shared_ptr<HttpPacket>;

	enum class Status
	{
		WAITING_FOR_PACKING,
		PAKING,
		PACKED
	};

	HttpPacket(uint32_t packetIndex, std::vector<uint8_t>& packetData);

	uint32_t _packetIndex = 0; // Порядковый номер пакета
	std::vector<uint8_t> _packetData;
	Status status;
};

//===============================================================================
//
//===============================================================================

class EventConDiscon
{
public:
	using Ptr = std::unique_ptr<EventConDiscon>;

	enum class EventType {
		CONNECT,
		DISCONNECT
	};

	EventType eventType;
};

//===============================================================================
//
//===============================================================================

class MTQueueOut
{
public:
	std::queue<HttpPacket::Ptr> queue;
	uint32_t lastSentPacketN = 0;     // Номер последнего добавленного в эту очередь пакета

	void PushPacket(std::vector<uint8_t>& data);
};

//===============================================================================
//
//===============================================================================

class MTQueueIn
{
public:
	std::vector<HttpPacket::Ptr> queue;
};

//===============================================================================
//
//===============================================================================

class MTQueueConDiscon
{
public:
	std::vector<EventConDiscon::Ptr> queue;
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class SConnectedClientLow
{
public:
	using Ptr = std::unique_ptr<SConnectedClientLow>;

	std::string login;
	std::string password;

	MTQueueIn packetsQueueIn;          // Очередь пакетов пришедших от клиента 
	uint32_t lastRecievedPacketN = 0;  // Номер последнего полученного с клиента пакета (защита от дублирования входящих пакетов)

	MTQueueOut packetsQueueOut;        // Очередь пакетов, которые нужно отослать клиенту
	uint32_t timestampLastRequest = 0; // Когда от этого клиента приходил последний запрос
};

//===============================================================================
//
//===============================================================================

class SConnectedClient
{
public:
	using Ptr = std::shared_ptr<SConnectedClient>;

	std::string _login;
	std::string _dbName;  // Имя база, с которой сейчас работает клиент
	bool _syncFinished = false; // Ставится в true, когда в _msgsQueueOut записаны все сообщения стартовой синхронизации и значит можно добавлять сообщения синхронизации с других клиентов
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать клиенту
	std::vector<DeserializationBuffer::Ptr> _msgsQueueIn;  // Очередь пришедших от клиента сообщений
};

//===============================================================================
//
//===============================================================================

class MTClientsLow
{
public:

	std::vector<SConnectedClientLow::Ptr> clients; // Низкоуровневая информация о подключенных клиентах
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class SHttpManager
{
public:
	MTClientsLow _mtClients;        // Низкоуровневая информация о подключенных клиентах
	MTQueueConDiscon _conDiscon;  // Очередь событий о подключении новых клиентов и отключении старых
};


//===============================================================================
// Сообщения из очереди исходящих сообщений переносит в очередь пакетов, оптимально объединяя или разбивая
// Также входящие пакеты перекладывает в очередь сообщений объединяя или разбивая
//===============================================================================

class SMessagesRepaker
{
public:
	SMessagesRepaker(STextsToolApp* app);
	void Update(double dt);

	STextsToolApp* _app = nullptr;
};


//===============================================================================
//
//===============================================================================

class STextsToolApp
{
public:
	STextsToolApp();
	void Update(double dt);

	std::vector<TextsDatabasePtr> _dbs;
	std::vector<SConnectedClient::Ptr> _clients;

	SClientMessagesMgr _messagesMgr;
	SHttpManager       _httpMgr;
	SMessagesRepaker   _messagesRepaker;
};

//===============================================================================
// FNV-1a algorithm https://softwareengineering.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed
//===============================================================================

uint64_t AddHash(uint64_t curHash, std::vector<uint8_t>& key, bool isFirstPart)  
{
	if (isFirstPart) {
		curHash = 14695981039346656037;
	}

	for (uint8_t el : key) {
		curHash = curHash ^ el;
		curHash *= 1099511628211;
	}
	return curHash;
}


