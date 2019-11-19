#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "SClientMessagesMgr.h"
#include "SHttpManager.h"
#include "SMessagesRepacker.h"

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

//===============================================================================
//
//===============================================================================

class SConnectedClient
{
public:
	SConnectedClient(const std::string& login, uint32_t sessionId);

	std::string _login;
	uint32_t _sessionId;
	std::string _dbName;  // Имя база, с которой сейчас работает клиент
	bool _syncFinished = false; // Ставится в true, когда в _msgsQueueOut записаны все сообщения стартовой синхронизации и значит можно добавлять сообщения синхронизации с других клиентов
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать клиенту
	std::vector<DeserializationBufferPtr> _msgsQueueIn;  // Очередь пришедших от клиента сообщений
};
using SConnectedClientPtr = std::shared_ptr<SConnectedClient>;


//===============================================================================
//
//===============================================================================

class STextsToolApp
{
public:
	STextsToolApp();
	void Update(double dt);

	std::vector<TextsDatabasePtr> _dbs;
	std::vector<SConnectedClientPtr> _clients;

	SClientMessagesMgr _messagesMgr;
	SHttpManager       _httpMgr;
	SMessagesRepacker   _messagesRepaker;
};

