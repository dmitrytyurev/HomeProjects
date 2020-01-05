#pragma once
#include <stdint.h>

const int HTTP_BUF_SIZE = 500000;
const bool NEED_PACK_PACKETS = false;

//-------------------------------------------------------------------------------


namespace AttributePropertyDataType // ��� ������ �������� �������
{
	enum : uint8_t
	{
		Translation_t = 0,  // ����� ������ �� �������������� ������
		CommonText_t = 1,   // ����� ������ ���������� (�� �������)
		Checkbox_t = 2,     // �������
		BaseText_t = 3,     // ������� �����
		Id_t = 4,           // Id-������ (������)
		CreationTimestamp_t = 5,  // ��������� �������� ������
		ModificationTimestamp_t = 6, // ��������� ���������� ������
	};
}

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
		ChangeAttributeInText = 112,   // ���������� �������� �������� � ������ (���� ���������� �������� ��� � ������, �� �� ��������; ���� ����������� ������ �����, �� ������� ��������� �� ������)
	};
}

//-------------------------------------------------------------------------------
// �������� ������� ��� ������������� ������������ ���������
//-------------------------------------------------------------------------------

struct TextsInterval
{
	uint32_t firstTextIdx = 0; // ������ ������� ������ ���������
	uint32_t textsNum = 0; // ���������� ������� � ���������
	uint64_t hash = 0;  // ��� ������ ������� ���������
};

//-------------------------------------------------------------------------------

inline void MakeKey(uint32_t tsModified, const std::string& textId, std::vector<uint8_t>& result)
{
	result.resize(sizeof(uint32_t) + textId.size());
	uint8_t* p = &result[0];

	*(reinterpret_cast<uint32_t*>(p)) = tsModified;
	memcpy(p + sizeof(uint32_t), textId.c_str(), textId.size());
}

//-------------------------------------------------------------------------------

inline bool IfKeyALess(const uint8_t* p1, int size1, const uint8_t* p2, int size2)
{
	if (*((uint32_t*)p1) < *((uint32_t*)p2)) {
		return true;
	}
	if (*((uint32_t*)p1) > *((uint32_t*)p2)) {
		return false;
	}
	p1 += sizeof(uint32_t);
	p2 += sizeof(uint32_t);
	while (true) {
		if (*p1 < *p2) {
			return true;
		}
		else {
			if (*p1 > *p2) {
				return false;
			}
		}
		--size1;
		--size2;
		if (size1 == 0 && size2 == 0) {
			return false;
		}
		if (size1 == 0) {
			return true;
		}
		if (size2 == 0) {
			return false;
		}
		++p1;
		++p2;
	}
	return false;
}
