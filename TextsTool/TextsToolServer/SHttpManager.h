#pragma once
#include <vector>
#include <stdint.h>
#include <functional>
#include <mutex>

#include "DeserializationBuffer.h"
#include "SerializationBuffer.h"
#include "SHttpManagerLow.h"


//===============================================================================
//
//===============================================================================

class HttpPacket
{
public:
	using Ptr = std::shared_ptr<HttpPacket>;

	enum class Status
	{
		// ---- Для пакетов с клиента
		RECEIVED, // Пакет пришёл с клиента
		// ---- Для пакетов на клиент
		WAITING_FOR_PACKING,  // Пакет добавлен в очередь на отсылку на клиент, но ещё не упакован
		PAKING,  // Пакету упаковывается
		PACKED  // Пакет упакован, можно посылать
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
	EventConnect(const std::string& login, uint32_t sessionId) : _login(login), _sessionId(sessionId) {}

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
	uint32_t _expectedClientPacketN = 0;  // Номер пакета в рамках данной сессии, который мы ждём от клиента (защита от дублирования входящих пакетов)

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
		Account(const std::string& login_, const std::string& password_) : login(login_), password(password_) {}
		std::string login;
		std::string password;
		uint32_t sessionId = 0;  // ID текущей сессии, если в _connections есть клиент с таким login. А если нету, значит здесь ID последней завершившейся сессии
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
	SHttpManager(std::function<void(const std::string&, uint32_t)> connectClient, std::function<void(const std::string&, uint32_t)> diconnectClient);
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
