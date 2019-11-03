#include "pch.h"

#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <ctime>
#include <algorithm>
#include <iterator>

#include "SHttpManager.h"
#include "Utils.h"


//const static uint32_t TIMEOUT_CLIENT_NOT_CONNECTED = 300;   // ������� � �������������, � ������ ����� �������� ����� � �������, ������� ��� ��������� ����������
const static uint32_t TIMEOUT_NO_SUCH_PACKET = 3000;        // ������� � �������������, � ������ ����� ������ ����������� ����� �� �������, �������� ��� ���
const static uint32_t TIMEOUT_NEXT_PACKET_READY = 0;        // ������� � �������������, � ������ ����� ������ ����������� ����� �� �������, � ��� ����� ��������� �����
const static uint32_t TIMEOUT_NEXT_PACKET_NOT_READY = 3000; // ������� � �������������, � ������ ����� ������ ����������� ����� �� �������, � ��� �� ����� ��������� �����
const static uint32_t TIMEOUT_DISCONNECT_CLIENT = 30;       // ������� ����� ������� ������ ��������� �������� �� ������� ������������ ���

//===============================================================================
//
//===============================================================================

HttpPacket::HttpPacket(std::vector<uint8_t>& packetData, Status status) : _packetData(packetData), _status(status)
{
}

//===============================================================================
//
//===============================================================================


HttpPacket::HttpPacket(DeserializationBuffer& request, Status status) : _status(status)
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

SHttpManager::SHttpManager(std::function<void(const std::string&, uint32_t)> connectClient, std::function<void(const std::string&, uint32_t)> diconnectClient) :
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

	// ��������� ������������ �� �������
	{
		uint32_t curTime = (uint32_t)std::time(0);
		MutexLock lock(_connections.mutex);

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

	// ��������� ���������
	{
		MutexLock lock(_connectQueue.mutex);

		for (auto& conDisconEvent : _connectQueue.queue) {
			_connectClient(conDisconEvent._login, conDisconEvent._sessionId);
		}
		_connectQueue.queue.resize(0);
	}
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
			MutexLock lock(_connectQueue.mutex);
			_connectQueue.queue.emplace_back(login, pAccount->sessionId);
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
			//response.Push(pAccount->sessionId);     ��������� �����������, ��� ��� ��������� �� ����� �� �������. ��������� ������ ������, ������ ������ ������ ������� reconnect
			//response.Push(TIMEOUT_CLIENT_NOT_CONNECTED);
			return;
		}
		// ������� ����� ���������� ������� �� ������� �������
		(*itClientLow)->_timestampLastRequest = (uint32_t)std::time(0);
		// ������� ������ �� ������� ������, ������� ������� ����� �� ������ (������ �������� ��������� ����� ��� �����)
		auto& queue = (*itClientLow)->_packetsQueueOut.queue;
		int elementsToErase = requestedPacketN - (*itClientLow)->_packetsQueueOut.lastPushedPacketN + queue.size() - 1;
		if (elementsToErase) {
			if (elementsToErase > (int)queue.size()) {
				elementsToErase = queue.size();
			}
			queue.erase(queue.begin(), queue.begin() + elementsToErase);
		}
		// ��������� ������ � ������� ������ � ������������� ������� requestedPacketN
		int requestedPacketIndex = requestedPacketN - (*itClientLow)->_packetsQueueOut.lastPushedPacketN + queue.size() - 1;
		if (requestedPacketIndex < 0 || requestedPacketIndex >= (int)queue.size() || queue[requestedPacketIndex]->_status != HttpPacket::Status::PACKED) {
			response.Push((uint8_t)NoSuchPacketYet);
			response.Push(TIMEOUT_NO_SUCH_PACKET);
			return;
		}
		// ��������, ����� �� �� ������� ��������� �� ������ �����
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
			//response.Push(pAccount->sessionId);     ��������� �����������, ��� ��� ��������� �� ����� �� �������. ��������� ������ ������, ������ ������ ������ ������� reconnect
			//response.Push(TIMEOUT_CLIENT_NOT_CONNECTED);
			return;
		}
		// ������� ����� ���������� ������� �� ������� �������
		(*itClientLow)->_timestampLastRequest = (uint32_t)std::time(0);
		// ������� � ������� ���������� �����
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

SConnectedClientLow::SConnectedClientLow(const std::string& login, uint32_t sessionId) : _login(login), _sessionId(sessionId)
{
	_timestampLastRequest = (uint32_t)std::time(0);
}

//===============================================================================
//
//===============================================================================

void SConnectedClientLow::reinit(uint32_t sessionId)
{
	_sessionId = sessionId;
	_lastRecievedPacketN = 0;
	_timestampLastRequest = (uint32_t)std::time(0);
	_packetsQueueIn.resize(0);
	_packetsQueueOut.lastPushedPacketN = UINT32_MAX;
	_packetsQueueOut.queue.resize(0);
}


