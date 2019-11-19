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
// ������������ �������� ������� ��������� �� ��������, ������������ ���� ��� ��������� ���� ���������, 
// ���������� �� ���� �������, ��������� ��������� � ������� �� ������� (��� ������������� ���� �������� -
// � ��� ����� � ���, �� ���� ������ ���������)
// ����� ������������ ������� ����������� � ����������. �� ��������� ����� ������/������� ������� 
// SConnectedClient. ��� ����� �������� ���������� � ������� ��������� � ��������� �������������.
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
	std::string _dbName;  // ��� ����, � ������� ������ �������� ������
	bool _syncFinished = false; // �������� � true, ����� � _msgsQueueOut �������� ��� ��������� ��������� ������������� � ������ ����� ��������� ��������� ������������� � ������ ��������
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // ������� ���������, ������� ����� �������� �������
	std::vector<DeserializationBufferPtr> _msgsQueueIn;  // ������� ��������� �� ������� ���������
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

