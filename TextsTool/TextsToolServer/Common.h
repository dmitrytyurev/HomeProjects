#pragma once
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




