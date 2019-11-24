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
		uint32_t textsInIntervalNum = 0;  // Количество текстов в интервале
		uint64_t hashOfKeys = 0;          // Хэш ключей текстов интервала
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
				uint32_t textsInIntervalNum = buf.GetUint32();    // Число текстов в интервале
				uint64_t hashOfKeys = buf.GetUint64();            // CRC64 ключей входящих в группу текстов
				intervals.emplace_back(textsInIntervalNum, hashOfKeys);
			}
		}
	}

	uint32_t id = 0;   // Id папки
	uint32_t tsModified = 0; // Ts изменения папки на клиенте
	std::vector<std::vector<uint8_t>> keys;      // Ключи, разбивающие тексты на интервалы. Ключ - бинарная строка: 4 байта ts + текстовый id-текста 
	std::vector<Interval> intervals;     // Параметры интервалов, задаваемых ключами (интервалов на один больше чем ключей)
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
	struct TextKey
	{
		std::vector<uint8_t> key;   // Ключ - бинарная строка: 4 байта ts + текстовый id-текста 
		TextTranslated* textRef = nullptr;    // Указатель на текст, для которого этот ключ
	};

	struct Interval
	{
		uint32_t firstTextIdx = 0; // Индекс первого текста интервала
		uint32_t textsNum = 0; // Количество текстов в интервале
		uint64_t hash = 0;  // Хэш ключей текстов интервала
	};

	static bool IfKeyALess(const uint8_t* p1, int size1, const uint8_t* p2, int size2);
	void MakeKey(uint32_t tsModified, const std::string& textId, std::vector<uint8_t>& result);
	void SaveToHistory(TextsDatabasePtr db, const std::string& login, uint32_t ts, const DeserializationBuffer& buf);
	// Разослать пакеты другим клиентам
	void SendToClients(const std::string& dbName, uint32_t ts, const DeserializationBuffer& buf, const std::string& loginOfLastModifier);
	TextsDatabasePtr GetDbPtrByDbName(const std::string& dbName);
	void FillDiffFolderInfo(SerializationBuffer* buffer, Folder& srvFolder, ClientFolder& cltFolder);

public:
	STextsToolApp* _app = nullptr;
};