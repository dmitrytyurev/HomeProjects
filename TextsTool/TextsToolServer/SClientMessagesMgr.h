#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"

class TextsDatabase;
class STextsToolApp;

//===============================================================================

class ClientFolder
{
public:
	struct Interval
	{
		Interval(uint32_t textsNum, uint64_t keysHash) : textsInIntervalNum(textsNum), hashOfKeys(keysHash) {}
		uint32_t textsInIntervalNum = 0;  // ���������� ������� � ���������
		uint64_t hashOfKeys = 0;          // ��� ������ ������� ���������
	};

	ClientFolder() {}
	ClientFolder(DeserializationBuffer& buf)
	{
		id = buf.GetUint32();
		tsModified = buf.GetUint32();
		uint32_t keysNum = buf.GetUint32();
		for (uint32_t keyIdx = 0; keyIdx < keysNum; ++keyIdx) {
			keys.emplace_back(buf.GetVector<uint8_t>());
		}
		if (keysNum > 0) {
			for (uint32_t intervalIdx = 0; intervalIdx < keysNum + 1; ++intervalIdx) {
				uint32_t textsInIntervalNum = buf.GetUint32();    // ����� ������� � ���������
				uint64_t hashOfKeys = buf.GetUint64();            // CRC64 ������ �������� � ������ �������
				intervals.emplace_back(textsInIntervalNum, hashOfKeys);
			}
		}
	}

	uint32_t id = 0;   // Id �����
	uint32_t tsModified = 0; // Ts ��������� ����� �� �������
	std::vector<std::vector<uint8_t>> keys;      // �����, ����������� ������ �� ���������. ���� - �������� ������: 4 ����� ts + ��������� id-������ 
	std::vector<Interval> intervals;     // ��������� ����������, ���������� ������� (���������� �� ���� ������ ��� ������)
};

//===============================================================================

class SClientMessagesMgr
{
public:
	SClientMessagesMgr(STextsToolApp* app);
	void Update(double dt);
	void AddPacketToClients(SerializationBufferPtr& bufPtr, const std::string& srcDbName);
	SerializationBufferPtr MakeSyncMessage(DeserializationBuffer& buf, TextsDatabase& db);
	SerializationBufferPtr MakeDatabasesListMessage();
	void ConnectClient(const std::string& login, uint32_t sessionId);    // ���������� �� HttpMgr::Update ��� ������� ������� �����������/����������. ������ SConnectedClient.
	void DisconnectClient(const std::string& login, uint32_t sessionId); // ���������� �� HttpMgr::Update ��� ������� ������� �����������/����������. ������� SConnectedClient.

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

private:
	struct TextKey
	{
		std::vector<uint8_t> key;   // ���� - �������� ������: 4 ����� ts + ��������� id-������ 
		TextTranslated* textRef = nullptr;    // ��������� �� �����, ��� �������� ���� ����
	};

	struct Interval
	{
		uint32_t firstTextIdx = 0; // ������ ������� ������ ���������
		uint32_t textsNum = 0; // ���������� ������� � ���������
		uint64_t hash = 0;  // ��� ������ ������� ���������
	};

	static bool IfKeyALess(const uint8_t* p1, int size1, const uint8_t* p2, int size2);
	void MakeKey(uint32_t tsModified, const std::string& textId, std::vector<uint8_t>& result);
	void SaveToHistory(TextsDatabasePtr db, const std::string& login, uint32_t ts, const DeserializationBuffer& buf);
	// ��������� ������ ������ ��������
	void SendToClients(const std::string& dbName, uint32_t ts, const DeserializationBuffer& buf, const std::string& loginOfLastModifier);
	TextsDatabasePtr GetDbPtrByDbName(const std::string& dbName);
	void FillDiffFolderInfo(SerializationBuffer* buffer, Folder& srvFolder, ClientFolder& cltFolder);

public:
	STextsToolApp* _app = nullptr;
};