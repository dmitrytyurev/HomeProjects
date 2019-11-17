#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"

class TextsDatabase;
class STextsToolApp;

class SClientMessagesMgr
{
public:
	SClientMessagesMgr(STextsToolApp* app);
	void Update(double dt);
	void AddPacketToClients(SerializationBufferPtr& bufPtr, const std::string& srcDbName);
	SerializationBufferPtr MakeSyncMessage(DeserializationBuffer& buf, TextsDatabase& db);
	SerializationBufferPtr MakeDatabasesListMessage();
	void ConnectClient(const std::string& login, uint32_t sessionId);    // Вызывается из HttpMgr::Update при разборе очереди подключений/отключений. Создаёт SConnectedClient.
	void DisconnectClient(const std::string& login, uint32_t sessionId); // Вызывается из HttpMgr::Update при разборе очереди подключений/отключений. Удаляет SConnectedClient.

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
	static bool IfKeyALess(const uint8_t* p1, int size1, const uint8_t* p2, int size2);
	void MakeKey(uint32_t tsModified, const std::string& textId, std::vector<uint8_t>& result);
	void SaveToHistory(TextsDatabasePtr db, const std::string& login, uint8_t ts, const DeserializationBuffer& buf);
	// Разослать пакеты другим клиентам
	void SendToClients(const std::string& dbName, uint8_t ts, const DeserializationBuffer& buf, const std::string& loginOfLastModifier);
	TextsDatabasePtr GetDbPtrByDbName(const std::string& dbName);

public:
	STextsToolApp* _app = nullptr;
};