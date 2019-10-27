#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "SClientMessagesMgr.h"
#include "SHttpManager.h"

class TextsDatabase;
class Folder;

//===============================================================================
//
//===============================================================================

enum class PacketDataType
{
	WholeMessages = 0,  // � ������ ���� ��� ��������� ����� ���������
	PartOfMessage = 1,  // � ������ ���� ������������� ���������
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

