#pragma once

const int HTTP_BUF_SIZE = 500000;
const bool NEED_PACK_PACKETS = false;

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
	EventRequestListOfDatabases = 16,       // ������ ������ ��� � ��������
};
