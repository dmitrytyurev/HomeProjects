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

	for (const auto& db : _dbs) {
		db->Update(dt);
	}
}

//===============================================================================
//
//===============================================================================

STextsToolApp::STextsToolApp(): _messagesMgr(this), _messagesRepaker(this)
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
	MutexLock lock(_app->_httpMgr._mtClients.mutex);
	for (auto& client : _app->_clients) {
		SConnectedClientLow* clLow = nullptr;
		for (auto& clientLow : _app->_httpMgr._mtClients.clients) {
			if (client->_login == clientLow->login) {
				clLow = clientLow.get();
				break;
			}
		}
		if (!clLow) {
			continue;
		}
		{
			MutexLock lock(_app->_httpMgr._mtClients.mutex);
			for (auto& buf : client->_msgsQueueOut) {
				clLow->packetsQueueOut.PushPacket(buf->buffer);
			}
			client->_msgsQueueOut.resize(0);
			for (auto& packetPtr : clLow->packetsQueueIn.queue) {
				client->_msgsQueueIn.push_back(std::make_unique<DeserializationBuffer>(packetPtr->_packetData));
			}
			clLow->packetsQueueIn.queue.resize(0);
		}
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
	queue.push(std::make_unique<HttpPacket>(lastSentPacketN++, data));
}


