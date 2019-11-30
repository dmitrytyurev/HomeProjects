#include <ctime>
#include <iostream>
#include <experimental/filesystem>
#include <algorithm>
#include <iterator>

#include "Utils.h"
#include "Common.h"
#include "DbSerializer.h"
#include "../SharedSrc/Shared.h"

//===============================================================================

void PeriodUpdater::SetPeriod(double period)
{
	_period = period;
}

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

STextsToolApp::STextsToolApp(): 
	_messagesMgr(this), 
	_messagesRepaker(this), 
	_httpMgr(std::bind(&SClientMessagesMgr::ConnectClient, _messagesMgr, std::placeholders::_1, std::placeholders::_2),
		     std::bind(&SClientMessagesMgr::DisconnectClient, _messagesMgr, std::placeholders::_1, std::placeholders::_2))
{
}

//===============================================================================

SConnectedClient::SConnectedClient(const std::string& login, uint32_t sessionId) : _login(login), _sessionId(sessionId)
{
}
