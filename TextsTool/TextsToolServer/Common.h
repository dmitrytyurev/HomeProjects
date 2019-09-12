#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "SClientMessagesMgr.h"

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
	enum EventType {
		CONNECT,
		DISCONNECT
	};

	EventType eventType;
	std::string login;  // Логин клиента, который подключается или отключается
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
	std::vector<EventConDiscon> queue;
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
	struct Account
	{
		std::string login;
		std::string password;
		uint32_t sessionId;  // ID текущей сессии, если в _mtClients есть клиент с таким login. А если нету, значит здесь ID последней завершившейся сессии
	};

	SHttpManager(std::function<void (const std::string&)> connectClient, std::function<void(const std::string&)> diconnectClient);
	void Update(double dt);

public:
	std::function<void(const std::string&)> _connectClient;
	std::function<void(const std::string&)> _diconnectClient;
	std::vector<Account> accounts;
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

class SConnectedClient
{
public:
	using Ptr = std::shared_ptr<SConnectedClient>;

	SConnectedClient(const std::string& login);

	std::string _login;
	std::string _dbName;  // Имя база, с которой сейчас работает клиент
	bool _syncFinished = false; // Ставится в true, когда в _msgsQueueOut записаны все сообщения стартовой синхронизации и значит можно добавлять сообщения синхронизации с других клиентов
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать клиенту
	std::vector<DeserializationBuffer::Ptr> _msgsQueueIn;  // Очередь пришедших от клиента сообщений
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
