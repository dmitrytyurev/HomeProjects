#include "pch.h"

#include <ctime>
#include <iostream>
#include <experimental/filesystem>
#include <algorithm>
#include <iterator>

#include "Utils.h"
#include "Common.h"
#include "DbSerializer.h"



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
				clLow->_packetsQueueOut.PushPacket(buf->buffer);
			}
		}
		client->_msgsQueueOut.resize(0);

		for (auto& packetPtr : clLow->_packetsQueueIn.queue) {
			client->_msgsQueueIn.push_back(std::make_unique<DeserializationBuffer>(packetPtr->_packetData));
		}
		clLow->_packetsQueueIn.queue.resize(0);
	}
}

//===============================================================================
//
//===============================================================================

HttpPacket::HttpPacket(uint32_t packetIndex, std::vector<uint8_t>& packetData): _packetIndex(packetIndex), _packetData(packetData)
{
}

//===============================================================================
//
//===============================================================================

void MTQueueOut::PushPacket(std::vector<uint8_t>& data)
{
	queue.emplace_back(std::make_unique<HttpPacket>(lastSentPacketN++, data));
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
	uint8_t requestType = request.GetUint<uint8_t>();

	switch (requestType)
	{
	case RequestConnect:
	{
		uint32_t sessionId = request.GetUint<uint32_t>();
		std::string login;
		request.GetString<uint8_t>(login);
		std::string password;
		request.GetString<uint8_t>(password);
		{
			MutexLock lock(_connections.mutex);
			MTConnections::Account* pAccount = FindAccount(login, password);
			if (!pAccount) {
				response.Push((uint8_t)WrongLoginOrPassword);
				return;
			}
			++(pAccount->sessionId);
			CreateClientLow(login, pAccount->sessionId);
			{
				MutexLock lock(_conDiscon.mutex);
				_conDiscon.queue.emplace_back(EventConDiscon::CONNECT, login, pAccount->sessionId);
			}
			response.Push((uint8_t)Connected);
			response.Push(pAccount->sessionId);
		}

		return;
	}
	break;
	case RequestPacket:
	{

	}
	break;
	case ProvidePacket:
	{

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
		(*result)->reinit();
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

void SConnectedClientLow::reinit()
{
	_lastRecievedPacketN = 0;
	_timestampLastRequest = 0;
	_packetsQueueIn.queue.resize(0);
	_packetsQueueOut.lastSentPacketN = 0;
	_packetsQueueOut.queue.resize(0);
}


