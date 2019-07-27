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
	void AddPacketToClients(SerializationBufferPtr& bufPtr, std::string& srcDbName);

	STextsToolApp* _app = nullptr;

private:
	TextsDatabase::Ptr GetDbPtrByDbName(const std::string& dbName);
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

class HttpPacket
{
public:
	using Ptr = std::unique_ptr<HttpPacket>;

	HttpPacket(uint32_t packetIndex, std::vector<uint8_t>& packetData);

	uint32_t _packetIndex = 0; // Порядковый номер пакета
	std::vector<uint8_t> _packetData;
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

	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class MTQueueIn
{
public:
	std::vector<HttpPacket::Ptr> queue;
	std::mutex mutex;
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

	std::vector<TextsDatabase::Ptr> _dbs;
	std::vector<SConnectedClient::Ptr> _clients;

	SClientMessagesMgr _messagesMgr;
	SHttpManager       _httpManager;
	SMessagesRepaker   _messagesRepaker;
};




