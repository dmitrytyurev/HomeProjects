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

enum class PacketDataType
{
	WholeMessages = 0,  // � ������ ���� ��� ��������� ����� ���������
	PartOfMessage = 1,  //  � ������ ���� ������������� ���������
};
inline bool operator==(const uint8_t& v1, const PacketDataType& v2) { return v1 == (uint8_t)v2; }
inline bool operator!=(const uint8_t& v1, const PacketDataType& v2) { return v1 != (uint8_t)v2; }

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
		RECEIVED,
		WAITING_FOR_PACKING,
		PAKING,
		PACKED
	};

	HttpPacket(std::vector<uint8_t>& packetData, Status status);
	HttpPacket(DeserializationBuffer& request, Status status);

	std::vector<uint8_t> _packetData;
	Status _status;
};

//===============================================================================
//
//===============================================================================

class EventConnect
{
public:
	EventConnect() {}
	EventConnect(const std::string& login, uint32_t sessionId): _login(login), _sessionId(sessionId) {}

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
	uint32_t lastPushedPacketN = UINT32_MAX;     // ����� ���������� ������������ � ��� ������� ������. �� ���� ����� ���������� ������ ������� � �������. � �� ���������� ��� �������� �� ������� �������, ������� ������� ����� �� ������ (�� �������� �������� �����)
	                                             // !!! ������� ������ ��� ����� ����
	void PushPacket(std::vector<uint8_t>& data, HttpPacket::Status status);
};

//===============================================================================
//
//===============================================================================

class MTQueueConnect
{
public:
	std::vector<EventConnect> queue;
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class SConnectedClientLow
{
public:
	using Ptr = std::unique_ptr<SConnectedClientLow>;
	SConnectedClientLow(const std::string& login, uint32_t sessionId);
	void reinit(uint32_t sessionId);

public:
	std::string _login;
	uint32_t _sessionId = 0; // ����� ���� MTConnections::Account::sessionId ��� �������� �������

	std::vector<HttpPacket::Ptr> _packetsQueueIn;          // ������� ������� ��������� �� ������� 
	uint32_t _lastRecievedPacketN = UINT32_MAX;  // ����� ���������� ����������� ������ � ������ ������ ������ ��� UINT32_MAX, ���� � ������ ������ ������ ��� �� ���� �������� ������ (������ �� ������������ �������� �������)

	MTQueueOut _packetsQueueOut;        // ������� �������, ������� ����� �������� �������
	uint32_t _timestampLastRequest = 0; // ����� �� ����� ������� �������� ��������� ������. ��� ������� �����������. !!! ������� ���������� � �������������
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
		WrongSession,
		ClientNotConnected,
		NoSuchPacketYet,
		Connected,
		PacketReceived,
		PacketSent
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
	MTConnections _connections;       // �������������� ���������� � ������������ ��������
	MTQueueConnect _connectQueue;     // ������� ������� � ����������� ����� ��������
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

