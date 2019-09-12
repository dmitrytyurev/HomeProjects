#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "SClientMessagesMgr.h"

class TextsDatabase;
class Folder;

//===============================================================================
//
//===============================================================================

enum EventType
{
	EventRequestSync = 0,          // ������ ������������� ��� ����������� ������ �������
	EventCreateFolder = 1,         // �������� �������� ��� �������
	EventDeleteFolder = 2,         // �������� �������� (������ �� ����� ������� � ��������� ���������)
	EventChangeFolderParent = 3,   // ��������� ������������� ��������
	EventRenameFolder = 4,         // �������������� ��������
	EventCreateAttribute = 5,      // �������� �������� �������
	EventDeleteAttribute = 6,      // �������� �������� �������
	EventRenameAttribute = 7,      // �������������� �������� �������
	EventChangeAttributeVis = 8,   // ��������� ������������� ����������� ������ �������� � ������� ������� ��� ��������� ��������
	EventCreateText = 9,           // �������� ������
	EventDeleteText = 10,           // �������� ������
	EventMoveTextToFolder = 11,    // ����� ������������ � ������ �����
	EventChangeBaseText = 12,      // ��������� �������� �����
	EventAddAttributeToText = 13,  // � ����� ��������� �������
	EventDelAttributeFromText = 14,  // �������� ������� �� ������
	EventChangeAttributeInText = 15, // ���������� �������� �������� � ������
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

class HttpPacket
{
public:
	using Ptr = std::shared_ptr<HttpPacket>;

	enum class Status
	{
		WAITING_FOR_PACKING,
		PAKING,
		PACKED
	};

	HttpPacket(uint32_t packetIndex, std::vector<uint8_t>& packetData);

	uint32_t _packetIndex = 0; // ���������� ����� ������
	std::vector<uint8_t> _packetData;
	Status status;
};

//===============================================================================
//
//===============================================================================

class EventConDiscon
{
public:
	enum EventType {
		CONNECT,
		DISCONNECT
	};

	EventType eventType;
	std::string login;  // ����� �������, ������� ������������ ��� �����������
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
};

//===============================================================================
//
//===============================================================================

class MTQueueIn
{
public:
	std::vector<HttpPacket::Ptr> queue;
};

//===============================================================================
//
//===============================================================================

class MTQueueConDiscon
{
public:
	std::vector<EventConDiscon> queue;
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
	struct Account
	{
		std::string login;
		std::string password;
		uint32_t sessionId;  // ID ������� ������, ���� � _mtClients ���� ������ � ����� login. � ���� ����, ������ ����� ID ��������� ������������� ������
	};

	SHttpManager(std::function<void (const std::string&)> connectClient, std::function<void(const std::string&)> diconnectClient);
	void Update(double dt);

public:
	std::function<void(const std::string&)> _connectClient;
	std::function<void(const std::string&)> _diconnectClient;
	std::vector<Account> accounts;
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

class SConnectedClient
{
public:
	using Ptr = std::shared_ptr<SConnectedClient>;

	SConnectedClient(const std::string& login);

	std::string _login;
	std::string _dbName;  // ��� ����, � ������� ������ �������� ������
	bool _syncFinished = false; // �������� � true, ����� � _msgsQueueOut �������� ��� ��������� ��������� ������������� � ������ ����� ��������� ��������� ������������� � ������ ��������
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // ������� ���������, ������� ����� �������� �������
	std::vector<DeserializationBuffer::Ptr> _msgsQueueIn;  // ������� ��������� �� ������� ���������
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
