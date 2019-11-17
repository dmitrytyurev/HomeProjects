#pragma once

const int HTTP_BUF_SIZE = 500000;
const bool NEED_PACK_PACKETS = false;

//===============================================================================
//
//===============================================================================

enum PacketDataType
{
	WholeMessages = 0,  // � ������ ���� ��� ��������� ����� ���������
	PartOfMessage = 1,  // � ������ ���� ������������� ���������
};

enum class ClientRequestTypes // ���� �������� �� �������
{
    RequestConnect = 0,
    RequestPacket = 1,
    ProvidePacket = 2
};

enum class AnswersToClient // ���� �������
{
    UnknownRequest = 0,
    WrongLoginOrPassword = 1,
    WrongSession = 2,
    ClientNotConnected = 3,
    NoSuchPacketYet = 4,
    Connected = 5,
    PacketReceived = 6,
    PacketSent = 7
};

//===============================================================================
//
//===============================================================================

enum EventType
{
	EventRequestSync = 0,               // ������ ������������� ��� ����������� ������ �������
	EventReplySync = 1,                 // ����� �� ������ EventRequestSync
	EventRequestListOfDatabases = 2,    // ������ ������ ��� � ��������
	EventReplyListOfDatabases = 3,      // ����� �� ������ EventRequestListOfDatabases

	// ���� �������, ���������� � ������� �� ������, � ����� ����������� � ������� �� ��������� ������� ������������ � ������ ����
	// ������������ � ������ ���� � �������, ������� �� ����� ���� ��������
	EventCreateFolder = 100,            // �������� �������� ��� �������
	EventDeleteFolder = 101,            // �������� �������� (������ �� ����� ������� � ��������� ���������)
	EventChangeFolderParent = 102,      // ��������� ������������� ��������
	EventRenameFolder = 103,            // �������������� ��������
	EventCreateAttribute = 104,         // �������� �������� �������
	EventDeleteAttribute = 105,         // �������� �������� �������
	EventRenameAttribute = 106,         // �������������� �������� �������
	EventChangeAttributeVis = 107,      // ��������� ������������� ����������� ������ �������� � ������� ������� ��� ��������� ��������
	EventCreateText = 108,              // �������� ������
	EventDeleteText = 109,              // �������� ������
	EventMoveTextToFolder = 110,        // ����� ������������ � ������ �����
	EventChangeBaseText = 111,          // ��������� �������� �����
	EventAddAttributeToText = 112,      // � ����� ��������� �������
	EventDelAttributeFromText = 113,    // �������� ������� �� ������
	EventChangeAttributeInText = 114,   // ���������� �������� �������� � ������
};
