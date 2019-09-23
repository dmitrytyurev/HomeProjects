#include "pch.h"

#include <ctime>
#include <iostream>
#include <experimental/filesystem>
#include <algorithm>
#include <iterator>

#include "Utils.h"
#include "Common.h"
#include "DbSerializer.h"

const static uint32_t TIMEOUT_CLIENT_NOT_CONNECTED = 300;   // “аймаут в миллисекундах, в ответе когда приходит пакет с клиента, которым уже разорвано соединение
const static uint32_t TIMEOUT_NO_SUCH_PACKET = 3000;        // “аймаут в миллисекундах, в ответе когда клиент запрашивает пакет на сервере, которого ещЄ нет
const static uint32_t TIMEOUT_NEXT_PACKET_READY = 0;        // “аймаут в миллисекундах, в ответе когда клиент запрашивает пакет на сервере, а уже готов следующий пакет
const static uint32_t TIMEOUT_NEXT_PACKET_NOT_READY = 3000; // “аймаут в миллисекундах, в ответе когда клиент запрашивает пакет на сервере, а ещЄ не готов следующий пакет

//===============================================================================
//
//===============================================================================

void PeriodUpdater::SetPeriod(double period)
{
	_period = period;
}

//===============================================================================
//
//===============================================================================

bool PeriodUpdater::IsTimeToProceed(double dt)
{
	_timeToProcess -= dt;
	bool isTimeToProceed = (_timeToProcess <= 0);

	if (isTimeToProceed) {
		_timeToProcess += _period;
	}
	return isTimeToProceed;
}




//===============================================================================
//
//===============================================================================

void STextsToolApp::Update(double dt)
{
	_messagesMgr.Update(dt);
	_messagesRepaker.Update(dt);
	_httpMgr.Update(dt);

	for (const auto& db : _dbs) {
		db->Update(dt);
	}
}

//===============================================================================
//
//===============================================================================

STextsToolApp::STextsToolApp(): 
	_messagesMgr(this), 
	_messagesRepaker(this), 
	_httpMgr(std::bind(&SClientMessagesMgr::ConnectClient, _messagesMgr, std::placeholders::_1, std::placeholders::_2),
		     std::bind(&SClientMessagesMgr::DisconnectClient, _messagesMgr, std::placeholders::_1, std::placeholders::_2))
{
}

//===============================================================================
//
//===============================================================================

TextsDatabasePtr SClientMessagesMgr::GetDbPtrByDbName(const std::string& dbName)
{
	for (const auto& db : _app->_dbs) {
		if (dbName == db->_dbName) {
			return db;
		}
	}
	return nullptr;
}


//===============================================================================
//
//===============================================================================

SMessagesRepaker::SMessagesRepaker(STextsToolApp* app): _app(app)
{
}

//===============================================================================
//
//===============================================================================

void SMessagesRepaker::Update(double dt)
{
	MutexLock lock(_app->_httpMgr._connections.mutex);

	for (auto& client : _app->_clients) {
		SConnectedClientLow* clLow = nullptr;
		for (auto& clientLow : _app->_httpMgr._connections.clients) {
			if (client->_login == clientLow->_login) {
				clLow = clientLow.get();
				break;
			}
		}
		if (!clLow) {
			continue;
		}

		if (client->_sessionId == clLow->_sessionId) {
			for (auto& buf : client->_msgsQueueOut) {
				clLow->_packetsQueueOut.PushPacket(buf->buffer, HttpPacket::Status::WAITING_FOR_PACKING);
			}
		}
		client->_msgsQueueOut.resize(0); // ќчистим, даже если не скопировали (это был старый SConnectedClient ведь SConnectedClientLow уже подключилс€ новый, так что из SConnectedClient не нужно было брать старые сообщени€, они уже не актуальны)

		if (client->_sessionId == clLow->_sessionId) { // ≈сли они не созвпадут, значит создалс€ новый SConnectedClientLow, а SConnectedClient ещЄ не успел пересоздатьс€, он сделает это на следующем Update и тогда мы снова придЄт сюда и скопируем пакеты уже в нового SConnectedClient
			for (auto& packetPtr : clLow->_packetsQueueIn) {
				client->_msgsQueueIn.push_back(std::make_unique<DeserializationBuffer>(packetPtr->_packetData));
			}
			clLow->_packetsQueueIn.resize(0);
		}
	}
}

//===============================================================================
//
//===============================================================================

HttpPacket::HttpPacket( std::vector<uint8_t>& packetData, Status status): _packetData(packetData), _status(status)
{
}

//===============================================================================
//
//===============================================================================


HttpPacket::HttpPacket(DeserializationBuffer& request, Status status): _status(status)
{
	uint32_t remainDataSize = request._buffer.size() - request.offset;
	_packetData.resize(remainDataSize);
	memcpy(_packetData.data(), request._buffer.data() + request.offset, remainDataSize);
}


//===============================================================================
//
//===============================================================================

void MTQueueOut::PushPacket(std::vector<uint8_t>& data, HttpPacket::Status status)
{
	++lastPushedPacketN;
	queue.emplace_back(std::make_unique<HttpPacket>(data, status));
}

//===============================================================================
//
//===============================================================================

SConnectedClient::SConnectedClient(const std::string& login, uint32_t sessionId): _login(login), _sessionId(sessionId)
{
}

//===============================================================================
//
//===============================================================================

SHttpManager::SHttpManager(std::function<void(const std::string&, uint32_t)> connectClient, std::function<void(const std::string&, uint32_t)> diconnectClient):
	_connectClient(connectClient), 
	_diconnectClient(diconnectClient),
	_sHttpManagerLow(std::bind(&SHttpManager::RequestProcessor, this, std::placeholders::_1, std::placeholders::_2))
{
}

//===============================================================================
//
//===============================================================================

void SHttpManager::Update(double dt)
{
	_sHttpManagerLow.Update(dt);

	MutexLock lock(_conDiscon.mutex);

	for (auto& conDisconEvent : _conDiscon.queue) {
		if (conDisconEvent._eventType == EventConDiscon::CONNECT) {
			_connectClient(conDisconEvent._login, conDisconEvent._sessionId);
		}
		else
			if (conDisconEvent._eventType == EventConDiscon::DISCONNECT) {
				_diconnectClient(conDisconEvent._login, conDisconEvent._sessionId);
			}
	}
	_conDiscon.queue.resize(0);
}

//===============================================================================
//
//===============================================================================

void SHttpManager::RequestProcessor(DeserializationBuffer& request, SerializationBuffer& response)
{
	std::string login;
	request.GetString<uint8_t>(login);
	std::string password;
	request.GetString<uint8_t>(password);
	uint32_t sessionId = request.GetUint<uint32_t>();
	uint32_t packetN = request.GetUint<uint32_t>();
	uint8_t requestType = request.GetUint<uint8_t>();

	MutexLock lock(_connections.mutex);

	MTConnections::Account* pAccount = FindAccount(login, password);
	if (!pAccount) {
		response.Push((uint8_t)WrongLoginOrPassword);
		return;
	}

	switch (requestType)
	{
	case RequestConnect:
		{
			++(pAccount->sessionId);
			CreateClientLow(login, pAccount->sessionId);
			{
				MutexLock lock(_conDiscon.mutex);
				_conDiscon.queue.emplace_back(EventConDiscon::CONNECT, login, pAccount->sessionId);
			}
			response.Push((uint8_t)Connected);
			response.Push(pAccount->sessionId);
			return;
		}
		break;
	case RequestPacket:
		{
			uint32_t requestedPacketN = request.GetUint<uint32_t>();
			if (pAccount->sessionId != sessionId) {
				response.Push((uint8_t)WrongSession);
				return;
			}
			auto itClientLow = std::find_if(std::begin(_connections.clients), std::end(_connections.clients), [login](const SConnectedClientLow::Ptr& el) { return el->_login == login; });
			if (itClientLow == std::end(_connections.clients)) {
				response.Push((uint8_t)ClientNotConnected);
				response.Push(pAccount->sessionId);
				response.Push(TIMEOUT_CLIENT_NOT_CONNECTED);
				return;
			}
			// —начала удалим из очереди пакеты, которые успешно дошли на клиент (клиент запросил следующий после них пакет)
			auto& queue = (*itClientLow)->_packetsQueueOut.queue;
			int elementsToErase = requestedPacketN - (*itClientLow)->_packetsQueueOut.lastPushedPacketN + queue.size() - 1;
			if (elementsToErase) {
				if (elementsToErase > (int)queue.size()) {
					elementsToErase = queue.size();
				}
				queue.erase(queue.begin(), queue.begin() + elementsToErase);
			}
			// –асчитаем индекс в очереди пакета с запрашиваемым номером requestedPacketN
			int requestedPacketIndex = requestedPacketN - (*itClientLow)->_packetsQueueOut.lastPushedPacketN + queue.size() - 1;
			if (requestedPacketIndex < 0 || requestedPacketIndex >= (int)queue.size() || queue[requestedPacketIndex]->_status != HttpPacket::Status::PACKED) {
				response.Push((uint8_t)NoSuchPacketYet);
				response.Push(TIMEOUT_NO_SUCH_PACKET);
				return;
			}
			// ѕроверим, готов ли на отсылку следующий по номеру пакет
			int nextPacketIndex = (requestedPacketN + 1) - (*itClientLow)->_packetsQueueOut.lastPushedPacketN + queue.size() - 1;
			bool isNextPacketReadyToSend = (nextPacketIndex >= 0 && nextPacketIndex < (int)queue.size() && queue[nextPacketIndex]->_status == HttpPacket::Status::PACKED);
			uint32_t timeoutToSend = isNextPacketReadyToSend ? TIMEOUT_NEXT_PACKET_READY : TIMEOUT_NEXT_PACKET_NOT_READY;
			response.Push((uint8_t)PacketSent);
			response.Push(timeoutToSend);
			response.PushBytes(queue[requestedPacketIndex]->_packetData.data(), queue[requestedPacketIndex]->_packetData.size());
			return;
		}
		break;
	case ProvidePacket:
		{
			if (pAccount->sessionId != sessionId) {
				response.Push((uint8_t)WrongSession);
				return;
			}
			auto itClientLow = std::find_if(std::begin(_connections.clients), std::end(_connections.clients), [login](const SConnectedClientLow::Ptr& el) { return el->_login == login; });
			if (itClientLow == std::end(_connections.clients)) {
				response.Push((uint8_t)ClientNotConnected);
				response.Push(pAccount->sessionId);
				response.Push(TIMEOUT_CLIENT_NOT_CONNECTED);
				return;
			}
			if ((*itClientLow)->_lastRecievedPacketN != UINT32_MAX && packetN > (*itClientLow)->_lastRecievedPacketN) {
				(*itClientLow)->_lastRecievedPacketN = packetN;
				(*itClientLow)->_packetsQueueIn.emplace_back(std::make_shared<HttpPacket>(request, HttpPacket::Status::RECEIVED));
			}
			response.Push((uint8_t)PacketReceived);
		}
		break;
	default:
		LogMsg("SHttpManager::RequestProcessor: unknown requestType");
		response.Push((uint8_t)UnknownRequest);
		return;
	}
}

//===============================================================================
//
//===============================================================================

MTConnections::Account* SHttpManager::FindAccount(const std::string& login, const std::string& password)
{
	auto result = std::find_if(std::begin(_connections._accounts), std::end(_connections._accounts), [login, password](const MTConnections::Account& el) { return el.login == login && el.password == password; });
	if (result == std::end(_connections._accounts)) {
		return nullptr;
	}

	return &*result;
}

//===============================================================================
//
//===============================================================================

void SHttpManager::CreateClientLow(const std::string& login, uint32_t sessionId)
{
	auto result = std::find_if(std::begin(_connections.clients), std::end(_connections.clients), [login](const SConnectedClientLow::Ptr& el) { return el->_login == login; });
	if (result != std::end(_connections.clients)) {
		(*result)->reinit(sessionId);
	}
	else {
		_connections.clients.emplace_back(std::make_unique<SConnectedClientLow>(login, sessionId));
	}
}

//===============================================================================
//
//===============================================================================

SConnectedClientLow::SConnectedClientLow(const std::string& login, uint32_t sessionId): _login(login), _sessionId(sessionId)
{
}

//===============================================================================
//
//===============================================================================

void SConnectedClientLow::reinit(uint32_t sessionId)
{
	_sessionId = sessionId;
	_sessionId = UINT32_MAX;
	_lastRecievedPacketN = 0;
	_timestampLastRequest = 0;
	_packetsQueueIn.resize(0);
	_packetsQueueOut.lastPushedPacketN = UINT32_MAX;
	_packetsQueueOut.queue.resize(0);
}


