#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "SClientMessagesMgr.h"
#include "SHttpManagerLow.h"

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
		NONE,
		CONNECT,
		DISCONNECT
	};

	EventConDiscon() {}
	EventConDiscon(EventType eventType, const std::string& login, uint32_t sessionId): _eventType(eventType), _login(login), _sessionId(sessionId) {}

	EventType _eventType = NONE;
	std::string _login;  // ����� �������, ������� ������������ ��� �����������
	uint32_t _sessionId; 
};

//===============================================================================
//
//===============================================================================

class MTQueueOut
{
public:
	std::vector<HttpPacket::Ptr> queue;
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
class Account;

class SConnectedClientLow
{
public:
	using Ptr = std::unique_ptr<SConnectedClientLow>;
	SConnectedClientLow(const std::string& login, uint32_t sessionId);
	void reinit();

public:
	std::string _login;
	uint32_t _sessionId = 0;

	MTQueueIn _packetsQueueIn;          // ������� ������� ��������� �� ������� 
	uint32_t _lastRecievedPacketN = 0;  // ����� ���������� ����������� � ������� ������ (������ �� ������������ �������� �������)

	MTQueueOut _packetsQueueOut;        // ������� �������, ������� ����� �������� �������
	uint32_t _timestampLastRequest = 0; // ����� �� ����� ������� �������� ��������� ������
};

//===============================================================================
//
//===============================================================================

class MTConnections
{
public:
	struct Account
	{
		std::string login;
		std::string password;
		uint32_t sessionId;  // ID ������� ������, ���� � _connections ���� ������ � ����� login. � ���� ����, ������ ����� ID ��������� ������������� ������
	};

	std::vector<SConnectedClientLow::Ptr> clients; // �������������� ���������� � ������������ ��������
	std::vector<Account> _accounts;  // ��������, � ������� ����� ������������ �������
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class SHttpManager
{
public:
	enum // ���� �������� �� �������
	{
		RequestConnect,
		RequestPacket,
		ProvidePacket
	};

	enum // ���� �������
	{
		UnknownRequest,
		WrongLoginOrPassword,
		Connected
	};

	SHttpManager(std::function<void (const std::string&, uint32_t)> connectClient, std::function<void(const std::string&, uint32_t)> diconnectClient);
	void Update(double dt);
	void RequestProcessor(DeserializationBuffer& request, SerializationBuffer& response); // ���������� �� ����������� ������, ����� ���������� http-������ � ������������ �����

private:
	MTConnections::Account* FindAccount(const std::string& login, const std::string& password);
	void CreateClientLow(const std::string& login, uint32_t sessionId);

public:
	SHttpManagerLow _sHttpManagerLow;
	std::function<void(const std::string&, uint32_t)> _connectClient;
	std::function<void(const std::string&, uint32_t)> _diconnectClient;
	MTConnections _connections;         // �������������� ���������� � ������������ ��������
	MTQueueConDiscon _conDiscon;     // ������� ������� � ����������� ����� �������� � ���������� ������
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

	SConnectedClient(const std::string& login, uint32_t sessionId);

	std::string _login;
	uint32_t _sessionId;
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

