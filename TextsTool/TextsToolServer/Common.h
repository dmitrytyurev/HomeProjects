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

class SClientMessagesMgr
{
public:
	SClientMessagesMgr(STextsToolApp* app);
	void Update(double dt);
	void AddPacketToClients(SerializationBufferPtr& bufPtr, const std::string& srcDbName);
	void ProcessSync(DeserializationBuffer& buf, TextsDatabase& db);
	void ConnectClient();    // ���������� �� HttpMgr::Update ��� ������� ������� �����������/����������. ������ SConnectedClient.
	void DisconnectClient(); // ���������� �� HttpMgr::Update ��� ������� ������� �����������/����������. ������� SConnectedClient.

	static bool ModifyDbRenameFolder(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts);
	static bool ModifyDbDeleteFolder(DeserializationBuffer& buf, TextsDatabase& db);
	static bool ModifyDbChangeFolderParent(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts);
	static bool ModifyDbDeleteAttribute(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts);
	static bool ModifyDbRenameAttribute(DeserializationBuffer& buf, TextsDatabase& db);
	static bool ModifyDbChangeAttributeVis(DeserializationBuffer& buf, TextsDatabase& db);
	static bool ModifyDbDeleteText(DeserializationBuffer& buf, TextsDatabase& db, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbMoveTextToFolder(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbChangeBaseText(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbAddAttributeToText(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbDelAttributeFromText(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	static bool ModifyDbChangeAttributeInText(DeserializationBuffer& buf, TextsDatabase& db, const std::string& modifierLogin, uint32_t ts, uint32_t offsInHistoryFile, uint32_t& prevTsModified, uint32_t& prevOffsModified);
	
	STextsToolApp* _app = nullptr;

private:
	void SaveToHistory(TextsDatabasePtr db, const std::string& login, uint8_t ts, const DeserializationBuffer& buf);
	void SendToClients(const std::string& dbName, uint8_t ts, const DeserializationBuffer& buf, const std::string& loginOfLastModifier);
	TextsDatabasePtr GetDbPtrByDbName(const std::string& dbName);
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
	bool _syncFinished = false; // �������� � true, ����� � _msgsQueueOut �������� ��� ��������� ��������� ������������� � ������ ����� ��������� ��������� ������������� � ������ ��������
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

	std::vector<TextsDatabasePtr> _dbs;
	std::vector<SConnectedClient::Ptr> _clients;

	SClientMessagesMgr _messagesMgr;
	SHttpManager       _httpMgr;
	SMessagesRepaker   _messagesRepaker;
};




