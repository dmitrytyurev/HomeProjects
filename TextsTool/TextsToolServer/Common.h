#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "SClientMessagesMgr.h"
#include "SHttpManagerLow.h"

class TextsDatabase;
class Folder;

//===============================================================================
//
//===============================================================================

enum class PacketDataType
{
	WholeMessages = 0,  // В пакете одно или несколько целых сообщений
	PartOfMessage = 1,  //  В пакете одно незавершённое сообщение
};
inline bool operator==(const uint8_t& v1, const PacketDataType& v2) { return v1 == (uint8_t)v2; }
inline bool operator!=(const uint8_t& v1, const PacketDataType& v2) { return v1 != (uint8_t)v2; }

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
		RECEIVED,
		WAITING_FOR_PACKING,
		PAKING,
		PACKED
	};

	HttpPacket(std::vector<uint8_t>& packetData, Status status);
	HttpPacket(DeserializationBuffer& request, Status status);

	std::vector<uint8_t> _packetData;
	Status _status;
};

//===============================================================================
//
//===============================================================================

class EventConnect
{
public:
	EventConnect() {}
	EventConnect(const std::string& login, uint32_t sessionId): _login(login), _sessionId(sessionId) {}

	std::string _login;  // Логин клиента, который подключается или отключается
	uint32_t _sessionId; 
};

//===============================================================================
//
//===============================================================================

class MTQueueOut
{
public:
	std::vector<HttpPacket::Ptr> queue;
	uint32_t lastPushedPacketN = UINT32_MAX;     // Номер последнего добавленного в эту очередь пакета. По нему можем определить номера пакетов в очереди. А их используем для удаления из очереди пакетов, которые успешно дошли на клиент (им запрошен следущий пакет)
	                                             // !!! Сделать логику для этого поля
	void PushPacket(std::vector<uint8_t>& data, HttpPacket::Status status);
};

//===============================================================================
//
//===============================================================================

class MTQueueConnect
{
public:
	std::vector<EventConnect> queue;
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class SConnectedClientLow
{
public:
	using Ptr = std::unique_ptr<SConnectedClientLow>;
	SConnectedClientLow(const std::string& login, uint32_t sessionId);
	void reinit(uint32_t sessionId);

public:
	std::string _login;
	uint32_t _sessionId = 0; // Копия поля MTConnections::Account::sessionId для быстрого доступа

	std::vector<HttpPacket::Ptr> _packetsQueueIn;          // Очередь пакетов пришедших от клиента 
	uint32_t _lastRecievedPacketN = UINT32_MAX;  // Номер последнего полученного пакета в рамках данной сессии или UINT32_MAX, если в рамках данной сессии ещё не были получены пакеты (защита от дублирования входящих пакетов)

	MTQueueOut _packetsQueueOut;        // Очередь пакетов, которые нужно отослать клиенту
	uint32_t _timestampLastRequest = 0; // Когда от этого клиента приходил последний запрос. Для разрыва соедиенения. !!! Сделать заполнение и использование
};

//===============================================================================
//
//===============================================================================

class MTConnections
{
public:
	struct Account
	{
		std::string login;
		std::string password;
		uint32_t sessionId;  // ID текущей сессии, если в _connections есть клиент с таким login. А если нету, значит здесь ID последней завершившейся сессии
	};

	std::vector<SConnectedClientLow::Ptr> clients; // Низкоуровневая информация о подключенных клиентах
	std::vector<Account> _accounts;  // Аккаунты, с которых могут подключаться клиенты
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class SHttpManager
{
public:
	enum // Типы запросов от клиента
	{
		RequestConnect,
		RequestPacket,
		ProvidePacket
	};

	enum // Коды ответов
	{
		UnknownRequest,
		WrongLoginOrPassword,
		WrongSession,
		ClientNotConnected,
		NoSuchPacketYet,
		Connected,
		PacketReceived,
		PacketSent
	};

	SHttpManager(std::function<void (const std::string&, uint32_t)> connectClient, std::function<void(const std::string&, uint32_t)> diconnectClient);
	void Update(double dt);
	void RequestProcessor(DeserializationBuffer& request, SerializationBuffer& response); // Вызывается из неосновного потока, должа обработать http-запрос и сформировать ответ

private:
	MTConnections::Account* FindAccount(const std::string& login, const std::string& password);
	void CreateClientLow(const std::string& login, uint32_t sessionId);

public:
	SHttpManagerLow _sHttpManagerLow;
	std::function<void(const std::string&, uint32_t)> _connectClient;
	std::function<void(const std::string&, uint32_t)> _diconnectClient;
	MTConnections _connections;       // Низкоуровневая информация о подключенных клиентах
	MTQueueConnect _connectQueue;     // Очередь событий о подключении новых клиентов
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

	SConnectedClient(const std::string& login, uint32_t sessionId);

	std::string _login;
	uint32_t _sessionId;
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

