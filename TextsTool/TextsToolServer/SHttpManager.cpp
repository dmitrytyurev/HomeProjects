#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <ctime>
#include <algorithm>
#include <iterator>

#include "SHttpManager.h"
#include "Utils.h"
#include "../SharedSrc/Shared.h"

const static uint32_t TIMEOUT_DISCONNECT_CLIENT = 30;       // Таймаут через сколько секунд неприхода запросов от клиента дисконнектим его

//===============================================================================

HttpPacket::HttpPacket(std::vector<uint8_t>& packetData, Status status) : _packetData(packetData), _status(status)
{
}

//===============================================================================


HttpPacket::HttpPacket(DeserializationBuffer& request, Status status) : _status(status)
{
	uint32_t remainDataSize = request._buffer.size() - request.offset;
	_packetData.resize(remainDataSize);
	memcpy(_packetData.data(), request._buffer.data() + request.offset, remainDataSize);
}


//===============================================================================

void MTQueueOut::PushPacket(std::vector<uint8_t>& data, HttpPacket::Status status)
{
	++lastPushedPacketN;
	queue.emplace_back(std::make_unique<HttpPacket>(data, status));
}

//===============================================================================

SHttpManager::SHttpManager(std::function<void(const std::string&, uint32_t)> connectClient, std::function<void(const std::string&, uint32_t)> diconnectClient) :
	_connectClient(connectClient),
	_diconnectClient(diconnectClient),
	_sHttpManagerLow(std::bind(&SHttpManager::RequestProcessor, this, std::placeholders::_1, std::placeholders::_2))
{
}

//===============================================================================

void SHttpManager::Update(double dt)
{
	_sHttpManagerLow.Update(dt);

	// Обработка дисконнектов по времени
	{
		uint32_t curTime = (uint32_t)std::time(0);
		Utils::MutexLock lock(_connections.mutex);

		for (auto it = _connections.clients.begin(); it != _connections.clients.end(); ) {
			if (curTime > (*it)->_timestampLastRequest + TIMEOUT_DISCONNECT_CLIENT)
			{
				_diconnectClient((*it)->_login, (*it)->_sessionId);
				it = _connections.clients.erase(it);
			}
			else
				++it;
		}
	}

	// Обработка коннектов
	{
		Utils::MutexLock lock(_connectQueue.mutex);

		for (auto& conDisconEvent : _connectQueue.queue) {
			_connectClient(conDisconEvent._login, conDisconEvent._sessionId);
		}
		_connectQueue.queue.resize(0);
	}
}

//===============================================================================

void SHttpManager::RequestProcessor(DeserializationBuffer& request, SerializationBuffer& response)
{
	std::string login;
	request.GetString8(login);
	std::string password;
	request.GetString8(password);
	uint8_t requestType = request.GetUint8();

	Utils::MutexLock lock(_connections.mutex);

	MTConnections::Account* pAccount = FindAccount(login, password);
	if (!pAccount) {
		response.PushUint8(AnswersToClient::WrongLoginOrPassword);
		return;
	}

	switch (requestType)
	{
	case ClientRequestTypes::RequestConnect:
	{
		Log("Packet from client: RequestConnect");
		++(pAccount->sessionId);
		CreateClientLow(login, pAccount->sessionId);
		{
			Utils::MutexLock lock(_connectQueue.mutex);
			_connectQueue.queue.emplace_back(login, pAccount->sessionId);
		}
		response.PushUint8(AnswersToClient::Connected);
		response.PushUint32(pAccount->sessionId);
		return;
	}
	break;
	case ClientRequestTypes::RequestPacket:
	{
//		Log("Packet from client: RequestPacket");
		uint32_t sessionId = request.GetUint32();
		uint32_t requestedPacketN = request.GetUint32();
		if (pAccount->sessionId != sessionId) {
			response.PushUint8(AnswersToClient::WrongSession);
			return;
		}
		auto itClientLow = std::find_if(std::begin(_connections.clients), std::end(_connections.clients), [login](const SConnectedClientLowPtr& el) { return el->_login == login; });
		if (itClientLow == std::end(_connections.clients)) {
			response.PushUint8(AnswersToClient::ClientNotConnected);
			//response.Push(pAccount->sessionId);     Сложилось впечатление, что эти параметры не нужны на клиенте. Получении такого ответа, клиент должен просто сделать reconnect
			//response.Push(TIMEOUT_CLIENT_NOT_CONNECTED);
			return;
		}
		// Обновим время последнего запроса от данного клиента
		(*itClientLow)->_timestampLastRequest = (uint32_t)std::time(0);
		// Сначала удалим из очереди пакеты, которые успешно дошли на клиент (клиент запросил следующий после них пакет)
		auto& queue = (*itClientLow)->_packetsQueueOut.queue;
		int elementsToErase = requestedPacketN - (*itClientLow)->_packetsQueueOut.lastPushedPacketN + queue.size() - 1;
		if (elementsToErase) {
			if (elementsToErase > (int)queue.size()) {
				elementsToErase = queue.size();
			}
			queue.erase(queue.begin(), queue.begin() + elementsToErase);
		}
		// Расчитаем индекс в очереди пакета с запрашиваемым номером requestedPacketN
		int requestedPacketIndex = requestedPacketN - (*itClientLow)->_packetsQueueOut.lastPushedPacketN + queue.size() - 1;
		if (requestedPacketIndex < 0 || requestedPacketIndex >= (int)queue.size() || (NEED_PACK_PACKETS && queue[requestedPacketIndex]->_status != HttpPacket::Status::PACKED)) {
			response.PushUint8(AnswersToClient::NoSuchPacketYet);
			return;
		}
		// Проверим, готов ли на отсылку следующий по номеру пакет
		int nextPacketIndex = (requestedPacketN + 1) - (*itClientLow)->_packetsQueueOut.lastPushedPacketN + queue.size() - 1;
		bool isNextPacketReadyToSend = (nextPacketIndex >= 0 && nextPacketIndex < (int)queue.size() && (queue[nextPacketIndex]->_status == HttpPacket::Status::PACKED || !NEED_PACK_PACKETS));
		response.PushUint8(AnswersToClient::PacketSent);
		response.PushUint8(isNextPacketReadyToSend);
		response.PushBytes(queue[requestedPacketIndex]->_packetData.data(), queue[requestedPacketIndex]->_packetData.size());
		return;
	}
	break;
	case ClientRequestTypes::ProvidePacket:
	{
		Log("Packet from client: ProvidePacket");
		uint32_t sessionId = request.GetUint32();
		uint32_t packetN = request.GetUint32();
		Log("  sessionId: " + std::to_string(sessionId) + " packetN: " + std::to_string(packetN));

		if (pAccount->sessionId != sessionId) {
			response.PushUint8(AnswersToClient::WrongSession);
			return;
		}
		auto itClientLow = std::find_if(std::begin(_connections.clients), std::end(_connections.clients), [login](const SConnectedClientLowPtr& el) { return el->_login == login; });
		if (itClientLow == std::end(_connections.clients)) {
			response.PushUint8(AnswersToClient::ClientNotConnected);
			//response.Push(pAccount->sessionId);     Сложилось впечатление, что эти параметры не нужны на клиенте. Получении такого ответа, клиент должен просто сделать reconnect
			//response.Push(TIMEOUT_CLIENT_NOT_CONNECTED);
			return;
		}
		// Обновим время последнего запроса от данного клиента
		(*itClientLow)->_timestampLastRequest = (uint32_t)std::time(0);
		// Положим в очередь полученный пакет
		if (packetN == (*itClientLow)->_expectedClientPacketN) {
			(*itClientLow)->_expectedClientPacketN++;
			(*itClientLow)->_packetsQueueIn.emplace_back(std::make_shared<HttpPacket>(request, HttpPacket::Status::RECEIVED));
		}
		response.PushUint8(AnswersToClient::PacketReceived);
	}
	break;
	default:
		Log("SHttpManager::RequestProcessor: unknown requestType");
		response.PushUint8(AnswersToClient::UnknownRequest);
		return;
	}
}

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

void SHttpManager::CreateClientLow(const std::string& login, uint32_t sessionId)
{
	auto result = std::find_if(std::begin(_connections.clients), std::end(_connections.clients), [login](const SConnectedClientLowPtr& el) { return el->_login == login; });
	if (result != std::end(_connections.clients)) {
		(*result)->reinit(sessionId);
	}
	else {
		_connections.clients.emplace_back(std::make_unique<SConnectedClientLow>(login, sessionId));
	}
}

//===============================================================================

SConnectedClientLow::SConnectedClientLow(const std::string& login, uint32_t sessionId) : _login(login), _sessionId(sessionId)
{
	_timestampLastRequest = (uint32_t)std::time(0);
}

//===============================================================================

void SConnectedClientLow::reinit(uint32_t sessionId)
{
	_sessionId = sessionId;
	_expectedClientPacketN = 0;
	_timestampLastRequest = (uint32_t)std::time(0);
	_packetsQueueIn.resize(0);
	_packetsQueueOut.lastPushedPacketN = UINT32_MAX;
	_packetsQueueOut.queue.resize(0);
}


