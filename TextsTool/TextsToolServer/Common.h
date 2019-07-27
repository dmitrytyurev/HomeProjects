#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"

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

class SClientMessagesMgr
{
public:
	SClientMessagesMgr(STextsToolApp* app);
	void Update(double dt);
	void AddPacketToClients(SerializationBufferPtr& bufPtr, std::string& srcDbName);

	STextsToolApp* _app = nullptr;

private:
	TextsDatabase::Ptr GetDbPtrByDbName(const std::string& dbName);
};

//===============================================================================
//
//===============================================================================

class MutexLock
{
public:
	MutexLock(std::mutex& mutex) { pMutex = &mutex; mutex.lock(); }
	~MutexLock() { pMutex->unlock();  }
private:
	std::mutex* pMutex = nullptr;
};

//===============================================================================
//
//===============================================================================

class HttpPacket
{
public:
	using Ptr = std::unique_ptr<HttpPacket>;

	HttpPacket(uint32_t packetIndex, std::vector<uint8_t>& packetData);

	uint32_t _packetIndex = 0; // ���������� ����� ������
	std::vector<uint8_t> _packetData;
};

//===============================================================================
//
//===============================================================================

class EventConDiscon
{
public:
	using Ptr = std::unique_ptr<EventConDiscon>;

	enum class EventType {
		CONNECT,
		DISCONNECT
	};

	EventType eventType;
};

//===============================================================================
//
//===============================================================================

class MTQueueOut
{
public:
	std::queue<HttpPacket::Ptr> queue;
	uint32_t lastSentPacketN = 0;     // ����� ���������� ������������ � ��� ������� ������

	void PushPacket(std::vector<uint8_t>& data);

	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class MTQueueIn
{
public:
	std::vector<HttpPacket::Ptr> queue;
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class MTQueueConDiscon
{
public:
	std::vector<EventConDiscon::Ptr> queue;
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

	MTQueueIn packetsQueueIn;          // ������� ������� ��������� �� ������� 
	uint32_t lastRecievedPacketN = 0;  // ����� ���������� ����������� � ������� ������ (������ �� ������������ �������� �������)

	MTQueueOut packetsQueueOut;        // ������� �������, ������� ����� �������� �������
	uint32_t timestampLastRequest = 0; // ����� �� ����� ������� �������� ��������� ������
};

//===============================================================================
//
//===============================================================================

class SConnectedClient
{
public:
	using Ptr = std::shared_ptr<SConnectedClient>;

	std::string _login;
	std::string _dbName;  // ��� ����, � ������� ������ �������� ������
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // ������� ���������, ������� ����� �������� �������
	std::vector<DeserializationBuffer::Ptr> _msgsQueueIn;  // ������� ��������� �� ������� ���������
};

//===============================================================================
//
//===============================================================================

class MTClientsLow
{
public:

	std::vector<SConnectedClientLow::Ptr> clients; // �������������� ���������� � ������������ ��������
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class SHttpManager
{
public:
	MTClientsLow _mtClients;        // �������������� ���������� � ������������ ��������
	MTQueueConDiscon _conDiscon;  // ������� ������� � ����������� ����� �������� � ���������� ������
};


//===============================================================================
// ��������� �� ������� ��������� ��������� ��������� � ������� �������, ���������� ��������� ��� ��������
// ����� �������� ������ ������������� � ������� ��������� ��������� ��� ��������
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

class STextsToolApp
{
public:
	STextsToolApp();
	void Update(double dt);

	std::vector<TextsDatabase::Ptr> _dbs;
	std::vector<SConnectedClient::Ptr> _clients;

	SClientMessagesMgr _messagesMgr;
	SHttpManager       _httpManager;
	SMessagesRepaker   _messagesRepaker;
};




