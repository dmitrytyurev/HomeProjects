#pragma once
#include <vector>
#include <stdint.h>
#include <functional>
#include <mutex>

#include "DeserializationBuffer.h"
#include "SerializationBuffer.h"
#include "SHttpManagerLow.h"


//===============================================================================
//
//===============================================================================

class HttpPacket
{
public:
	using Ptr = std::shared_ptr<HttpPacket>;

	enum class Status
	{
		// ---- ��� ������� � �������
		RECEIVED, // ����� ������ � �������
		// ---- ��� ������� �� ������
		WAITING_FOR_PACKING,  // ����� �������� � ������� �� ������� �� ������, �� ��� �� ��������
		PAKING,  // ������ �������������
		PACKED  // ����� ��������, ����� ��������
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
	EventConnect(const std::string& login, uint32_t sessionId) : _login(login), _sessionId(sessionId) {}

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
	uint32_t _expectedClientPacketN = 0;  // ����� ������ � ������ ������ ������, ������� �� ��� �� ������� (������ �� ������������ �������� �������)

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
		Account(const std::string& login_, const std::string& password_) : login(login_), password(password_) {}
		std::string login;
		std::string password;
		uint32_t sessionId = 0;  // ID ������� ������, ���� � _connections ���� ������ � ����� login. � ���� ����, ������ ����� ID ��������� ������������� ������
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
	SHttpManager(std::function<void(const std::string&, uint32_t)> connectClient, std::function<void(const std::string&, uint32_t)> diconnectClient);
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
