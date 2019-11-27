#pragma once
#include <stdint.h>

const int HTTP_BUF_SIZE = 500000;
const bool NEED_PACK_PACKETS = false;

//-------------------------------------------------------------------------------

namespace FolderDataTypeForSyncMsg // ��� ��������� � ������������ ����: ��� �������� ������������ ��������
{
	enum : uint8_t
	{
		AllTexts = 0,  // ������������ ���������� ��� ���� ������� �������� (��������� �� ����)
		DiffTexts = 1, // ������������ ���������� �� ������������ ������� ��������, �������� �� �������
	};
}

//-------------------------------------------------------------------------------

namespace PacketDataType
{
	enum : uint8_t
	{
		WholeMessages = 0,  // � ������ ���� ��� ��������� ����� ���������
		PartOfMessage = 1,  // � ������ ���� ������������� ���������
	};
}

//-------------------------------------------------------------------------------

namespace ClientRequestTypes { // ���� �������� �� �������
	enum : uint8_t  
	{
		RequestConnect = 0,
		RequestPacket = 1,
		ProvidePacket = 2
	};
}

//-------------------------------------------------------------------------------

namespace AnswersToClient // ���� �������
{
	enum : uint8_t 
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
}

//-------------------------------------------------------------------------------

namespace EventType
{
	enum : uint8_t
	{
		RequestSync = 0,               // Clt->Srv: ������ ������������� ��� ����������� ������ �������
		ReplySync = 1,                 // Srv->Clt: ����� �� ������ EventRequestSync
		RequestListOfDatabases = 2,    // Clt->Srv: ������ ������ ��� � ��������
		ReplyListOfDatabases = 3,      // Srv->Clt: ����� �� ������ RequestListOfDatabases
		ChangeDataBase = 4,            // Srv->Clt: ��������� �� ��������� ���� (������ ������������ ��� ��� loopback) (��. ���� ������� ������� � ������ 100)

		// ���� �������, ���������� � ������� �� ������, � ����� ����������� � ������� �� ��������� ������� ������������ � ������ ����
		// ������������ � ������ ���� � �������, ������� �� ����� ���� ��������
		// ���������� �������� ��������, ������� �� ������� ���������� ������ ����� ������ � �������
		CreateFolder = 100,            //*�������� �������� ��� �������
		DeleteFolder = 101,            //*�������� �������� (������ �� ����� ������� � ��������� ���������)
		ChangeFolderParent = 102,      // ��������� ������������� ��������
		RenameFolder = 103,            // �������������� ��������
		CreateAttribute = 104,         //*�������� �������� �������
		DeleteAttribute = 105,         //*�������� �������� �������
		RenameAttribute = 106,         // �������������� �������� �������
		ChangeAttributeVis = 107,      // ��������� ������������� ����������� ������ �������� � ������� ������� ��� ��������� ��������
		CreateText = 108,              // �������� ������
		DeleteText = 109,              // �������� ������
		MoveTextToFolder = 110,        // ����� ������������ � ������ �����
		ChangeBaseText = 111,          // ��������� �������� �����
		AddAttributeToText = 112,      // � ����� ��������� �������
		DelAttributeFromText = 113,    // �������� ������� �� ������
		ChangeAttributeInText = 114,   // ���������� �������� �������� � ������
	};
}
