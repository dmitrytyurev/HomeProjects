#include "pch.h"

#include <iostream>
#include <ctime>
#include <iostream>
#include <experimental/filesystem>
#include <algorithm>
#include <iterator>

#include "Common.h"
#include "DbSerializer.h"
#include "SClientMessagesMgr.h"
#include "Utils.h"


//===============================================================================
//
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
		id = buf.GetUint<uint32_t>();
		tsModified = buf.GetUint<uint32_t>();
		uint32_t keysNum = buf.GetUint<uint32_t>();
		for (uint32_t keyIdx = 0; keyIdx < keysNum; ++keyIdx) {
			keys.emplace_back(buf.GetVector<uint8_t>());
		}
		for (uint32_t intervalIdx = 0; intervalIdx < keysNum + 1; ++intervalIdx) {
			uint32_t textsInIntervalNum = buf.GetUint<uint32_t>();    // Число текстов в интервале
			uint64_t hashOfKeys = buf.GetUint<uint64_t>();            // CRC64 ключей входящих в группу текстов
			intervals.emplace_back(textsInIntervalNum, hashOfKeys);
		}
	}

	uint32_t id = 0;   // Id папки
	uint32_t tsModified = 0; // Ts изменения папки на клиенте
	std::vector<std::vector<uint8_t>> keys;      // Ключи, разбивающие тексты на интервалы. Ключ - бинарная строка: 4 байта ts + текстовый id-текста 
	std::vector<Interval> intervals;     // Параметры интервалов, задаваемых ключами (интервалов на один больше чем ключей)
};

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::SaveToHistory(TextsDatabasePtr db, const std::string& login, uint8_t ts, const DeserializationBuffer& bufIn)
{
	auto& bufOut = db->GetHistoryBuffer();
	bufOut.PushStringWithoutZero<uint8_t>(login);
	bufOut.Push(ts);
	bufOut.Push(bufIn, true); // Заберём все данные из буфера, не важно сколько было из него уже прочитано // !!! Как это по ссылке на константу вызывается неконстантная функция??
}

//===============================================================================
// Разослать пакеты другим клиентам
//===============================================================================

void SClientMessagesMgr::SendToClients(const std::string& dbName, uint8_t ts, const DeserializationBuffer& buf, const std::string& loginOfLastModifier)
{
	auto bufPtr = std::make_shared<SerializationBuffer>();
	bufPtr->PushStringWithoutZero<uint8_t>(loginOfLastModifier);
	bufPtr->Push(ts);
	bufPtr->Push(buf, true); // Заберём все данные из буфера, не важно сколько было из него уже прочитано
	AddPacketToClients(bufPtr, dbName);
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::Update(double dt)
{
	for (auto& client : _app->_clients) {
		TextsDatabasePtr db = GetDbPtrByDbName(client->_dbName);
		for (const auto& buf : client->_msgsQueueIn) {
			uint8_t actionType = buf->GetUint<uint8_t>();
			uint32_t ts = GetTime();
			switch (actionType) {
			case EventRequestSync:
			{
				buf->GetString<uint8_t>(client->_dbName);
				TextsDatabasePtr db2 = GetDbPtrByDbName(client->_dbName);  // !!! Вероятно, тут надо проверить наличие базы и если нету, то запустить фоновую загрузку, а выполнение запроса отложить
				SerializationBufferPtr bufPtr = MakeSyncMessage(*buf, *db2); // Сформировать сообщение клиенту - для синхронизации его базы (он подключился)
				client->_msgsQueueOut.emplace_back(bufPtr);
				client->_syncFinished = true;
			}
			break;
			case EventCreateFolder:
			{
				db->_folders.emplace_back();                            // Изменения в базе
				Folder& folder = db->_folders.back();
				folder.CreateFromPacket(*buf, ts, db->_newFolderId++);
				folder.SaveToHistory(db, client->_login);               // Запись в файл истории
				SerializationBufferPtr bufPtr = folder.SaveToPacket(client->_login);  // Разослать пакеты другим клиентам 
				AddPacketToClients(bufPtr, client->_dbName);
			}
			break;
			case EventDeleteFolder:
			{
				bool isSucces = ModifyDbDeleteFolder(*buf, *db);
				SaveToHistory(db, client->_login, ts, *buf);          // Запись в файл истории
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);     // Разослать пакеты другим клиентам
				}
			}
			break;
			case EventChangeFolderParent:
			{
				bool isSucces = ModifyDbChangeFolderParent(*buf, *db, ts);
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);     // Разослать пакеты другим клиентам
				}
			}
			break;
			case EventRenameFolder:
			{
				bool isSucces = ModifyDbRenameFolder(*buf, *db, ts);              // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);         // Разослать пакеты другим клиентам
				}
			}
			break;
			case EventCreateAttribute:
			{
				db->_attributeProps.emplace_back();                 // Изменения в базе
				AttributeProperty& ap = db->_attributeProps.back();
				ap.CreateFromPacket(*buf, ts, db->_newAttributeId++);
				ap.SaveToHistory(db, client->_login);               // Запись в файл истории
				SerializationBufferPtr bufPtr = ap.SaveToPacket(client->_login);  // Разослать пакеты другим клиентам 
				AddPacketToClients(bufPtr, client->_dbName);
				db->_newAttributeId = db->_attributeProps.back().id + 1;
			}
			break;
			case EventDeleteAttribute:
			{
				bool isSucces = ModifyDbDeleteAttribute(*buf, *db, ts);           // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);         // Разослать пакеты другим клиентам			
				}
			}
			break;
			case EventRenameAttribute:
			{
				bool isSucces = ModifyDbRenameAttribute(*buf, *db);               // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);         // Разослать пакеты другим клиентам			
				}
			}
			break;
			case EventChangeAttributeVis:
			{
				bool isSucces = ModifyDbChangeAttributeVis(*buf, *db);            // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);         // Разослать пакеты другим клиентам			
				}
			}
			break;
			case EventCreateText:
			{
				uint32_t folderId = buf->GetUint<uint32_t>();     // Изменения в базе
				std::string textId;
				buf->GetString<uint8_t>(textId);
				auto& f = db->_folders;
				auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
				if (result != std::end(f)) {
					result->texts.emplace_back();
					TextTranslated& tt = *(result->texts.back());
					tt.id = textId;
					tt.loginOfLastModifier = client->_login;
					tt.timestampCreated = ts;
					tt.timestampModified = ts;
					tt.offsLastModified = db->GetCurrentPosInHistoryFile();
					tt.SaveToHistory(db, folderId);                             // Запись в файл истории
					SerializationBufferPtr bufPtr = tt.SaveToPacket(folderId, client->_login);  // Разослать пакеты другим клиентам 
					AddPacketToClients(bufPtr, client->_dbName);
				}
				else {
					Log("SClientMessagesMgr::Update: EventCreateText: folder not found");
				}
			}
			break;
			case EventDeleteText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				bool isSucces = ModifyDbDeleteText(*buf, *db, prevTsModified, prevOffsModified);                 // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);   // Запись в файл истории
				db->GetHistoryBuffer().Push(prevTsModified);
				db->GetHistoryBuffer().Push(prevOffsModified);
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
				}
			}
			break;
			case EventMoveTextToFolder:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				bool isSucces = ModifyDbMoveTextToFolder(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);               // Запись в файл истории
				db->GetHistoryBuffer().Push(prevTsModified);
				db->GetHistoryBuffer().Push(prevOffsModified);
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
				}
			}
			break;
			case EventChangeBaseText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				bool isSucces = ModifyDbChangeBaseText(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);               // Запись в файл истории
				db->GetHistoryBuffer().Push(prevTsModified);
				db->GetHistoryBuffer().Push(prevOffsModified);
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
				}
			}
			break;
			case EventAddAttributeToText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				uint32_t keepOffset = buf->offset;
				bool isSucces = ModifyDbAddAttributeToText(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				buf->offset = keepOffset; // Восстнавливаем буфер на состояние "прочитан только тип операции"
				auto& historyBuf = db->GetHistoryBuffer();                     // Запись в файл истории
				historyBuf.PushStringWithoutZero<uint8_t>(client->_login);
				historyBuf.Push(ts);
				historyBuf.Push(actionType);
				historyBuf.Push(prevTsModified);
				historyBuf.Push(prevOffsModified);
				historyBuf.Push(*buf, false); // Заберём только непрочитанные данные (всё кроме типа операции)
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
				}
			}
			break;
			case EventDelAttributeFromText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				uint32_t keepOffset = buf->offset;
				bool isSucces = ModifyDbDelAttributeFromText(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				buf->offset = keepOffset; // Восстнавливаем буфер на состояние "прочитан только тип операции"
				auto& historyBuf = db->GetHistoryBuffer();                     // Запись в файл истории
				historyBuf.PushStringWithoutZero<uint8_t>(client->_login);
				historyBuf.Push(ts);
				historyBuf.Push(actionType);
				historyBuf.Push(prevTsModified);
				historyBuf.Push(prevOffsModified);
				historyBuf.Push(*buf, false); // Заберём только непрочитанные данные (всё кроме типа операции)
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
				}
			}
			break;
			case EventChangeAttributeInText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				uint32_t keepOffset = buf->offset;
				bool isSucces = ModifyDbChangeAttributeInText(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				buf->offset = keepOffset; // Восстнавливаем буфер на состояние "прочитан только тип операции"
				auto& historyBuf = db->GetHistoryBuffer();                     // Запись в файл истории
				historyBuf.PushStringWithoutZero<uint8_t>(client->_login);
				historyBuf.Push(ts);
				historyBuf.Push(actionType);
				historyBuf.Push(prevTsModified);
				historyBuf.Push(prevOffsModified);
				historyBuf.Push(*buf, false); // Заберём только непрочитанные данные (всё кроме типа операции)
				if (isSucces) {
					SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
				}
			}
			break;

			/*
			   OffsModified
				   - В SaveToHistory пишется во все события над текстами кроме создания
						- Сначала сохраняем инфу о событии в файл-истории включая значение offsLastModified
						- А потом в объекте offsLastModified = смещение перед началом записи события	      !!!!!!!! Это делаем в создании текста тоже, хотя offset туда не сохраняем !!!!!!
				   - В SaveToPacket не пишется - на клиенте оно не нужно
			*/

			default:
				ExitMsg("Wrong actionType");
			}
		}
		client->_msgsQueueIn.resize(0); // Не используем clear, чтобы гарантировать отсутствие релокации
	}
}

//===============================================================================
//
//===============================================================================

bool SClientMessagesMgr::ModifyDbDeleteFolder(DeserializationBuffer& buf, TextsDatabase& db)
{
	uint32_t folderId = buf.GetUint<uint32_t>();
	auto& f = db._folders;                                 // Изменения в базе
	auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
	if (result != std::end(f)) {
		f.erase(result);
	}
	else {
		Log("SClientMessagesMgr::ModifyDbDeleteFolder: folder not found");
		return false;
	}
	return true;
}

//===============================================================================
//
//===============================================================================

bool SClientMessagesMgr::ModifyDbChangeFolderParent(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts)
{
	uint32_t folderId = buf.GetUint<uint32_t>();
	uint32_t newParentFolderId = buf.GetUint<uint32_t>();
	auto& f = db._folders;                                  // Изменения в базе
	auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
	if (result != std::end(f)) {
		result->parentId = newParentFolderId;
		result->timestampModified = ts;
	}
	else {
		Log("SClientMessagesMgr::ModifyDbChangeFolderParent: folder not found");
		return false;
	}
	return true;
}

//===============================================================================
//
//===============================================================================


bool SClientMessagesMgr::ModifyDbRenameFolder(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts)
{
	uint32_t folderId = buf.GetUint<uint32_t>();
	std::string newFolderName;
	buf.GetString<uint8_t>(newFolderName);
	auto& f = db._folders;
	auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
	if (result != std::end(f)) {
		result->name = newFolderName;
		result->timestampModified = ts;
	}
	else {
		Log("SClientMessagesMgr::ModifyDbRenameFolder: folder not found");
		return false;
	}
	return true;
}

//===============================================================================
//
//===============================================================================

bool SClientMessagesMgr::ModifyDbDeleteAttribute(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts)
{
	uint8_t attributeId = buf.GetUint<uint8_t>();
	uint8_t visPosOfDeletedAttr = 0;                   // Изменения в базе
	auto& ap = db._attributeProps;
	auto result = std::find_if(std::begin(ap), std::end(ap), [attributeId](const AttributeProperty& el) { return el.id == attributeId; });
	if (result != std::end(ap)) {
		visPosOfDeletedAttr = result->visiblePosition;
		ap.erase(result);
	}
	else {
		Log("SClientMessagesMgr::Update::ActionDeleteAttribute: attribute id not found");
		return false;
	}
	for (auto it = ap.begin(); it != ap.end(); ) {  // Сдвинуть позицию всех атрибутов справа от удаляемого
		if (it->visiblePosition > visPosOfDeletedAttr) {
			--(it->visiblePosition);
		}
	}
	for (auto& folder : db._folders) {  // Удалить атрибуты с таким id из всех текстов
		for (auto& text : folder.texts) {
			auto& attribs = text->attributes;
			int prevSize = attribs.size();
			attribs.erase(std::remove_if(attribs.begin(), attribs.end(), [attributeId](const auto& el) { return el.id == attributeId; }), attribs.end());
			if (attribs.size() != prevSize) {
				text->timestampModified = ts;
				folder.timestampModified = ts;
			}
		}
	}
	return true;
}

//===============================================================================
//
//===============================================================================

bool SClientMessagesMgr::ModifyDbRenameAttribute(DeserializationBuffer& buf, TextsDatabase& db)
{
	uint8_t attributeId = buf.GetUint<uint8_t>();
	std::string newAttributeName;
	buf.GetString<uint8_t>(newAttributeName);
	auto& ap = db._attributeProps;
	auto result = std::find_if(std::begin(ap), std::end(ap), [attributeId](const AttributeProperty& el) { return el.id == attributeId; });
	if (result != std::end(ap)) {
		result->name = newAttributeName;
	}
	else {
		Log("ModifyDbRenameAttribute: attribute id not found");
		return false;
	}
	return true;
}

//===============================================================================
//
//===============================================================================

bool SClientMessagesMgr::ModifyDbChangeAttributeVis(DeserializationBuffer& buf, TextsDatabase& db)
{
	uint8_t attributeId = buf.GetUint<uint8_t>();
	uint8_t newVisiblePosition = buf.GetUint<uint8_t>();
	uint8_t newVisibilityFlag = buf.GetUint<uint8_t>();
	auto& ap = db._attributeProps;
	auto result = std::find_if(std::begin(ap), std::end(ap), [attributeId](const AttributeProperty& el) { return el.id == attributeId; });
	if (result != std::end(ap)) {
		result->visiblePosition = newVisiblePosition;
		result->isVisible = static_cast<bool>(newVisibilityFlag);
	}
	else {
		Log("ModifyDbChangeAttributeVis: attribute id not found");
		return false;
	}
	return true;
}

//===============================================================================
//
//===============================================================================

bool SClientMessagesMgr::ModifyDbDeleteText(DeserializationBuffer& buf, TextsDatabase& db, uint32_t& prevTsModified, uint32_t& prevOffsModified)
{
	std::string textId;
	buf.GetString<uint8_t>(textId);
	for (auto& f : db._folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslated::Ptr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			prevTsModified = (*result)->timestampModified;
			prevOffsModified = (*result)->offsLastModified;
			f.texts.erase(result);
			return true;
		}
	}
	Log("ModifyDbDeleteText: text not found by id");
	return false;
}

//===============================================================================
//
//===============================================================================

bool SClientMessagesMgr::ModifyDbMoveTextToFolder(
	DeserializationBuffer& buf,
	TextsDatabase& db,
	const std::string& modifierLogin,
	uint32_t ts,
	uint32_t offsInHistoryFile,
	uint32_t& prevTsModified,
	uint32_t& prevOffsModified)
{
	std::string textId;
	buf.GetString<uint8_t>(textId);
	uint32_t newFolderId = buf.GetUint<uint32_t>();

	for (auto& f : db._folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslated::Ptr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			TextTranslated::Ptr tmpTextPtr = *result;
			f.texts.erase(result);
			f.timestampModified = ts;
			for (auto& f2 : db._folders) {
				if (f2.id == newFolderId) {
					f2.texts.emplace_back(tmpTextPtr);
					f2.timestampModified = ts;
					tmpTextPtr->loginOfLastModifier = modifierLogin;
					prevTsModified = tmpTextPtr->timestampModified;
					tmpTextPtr->timestampModified = ts;
					prevOffsModified = tmpTextPtr->offsLastModified;
					tmpTextPtr->offsLastModified = offsInHistoryFile;
					return true;
				}
			}
			Log("ModifyDbMoveTextToFolder: folder not found");
			return false;
		}
	}
	Log("ModifyDbMoveTextToFolder: text not found by id");
	return false;
}

//===============================================================================
//
//===============================================================================

bool  SClientMessagesMgr::ModifyDbChangeBaseText(
	DeserializationBuffer& buf,
	TextsDatabase& db,
	const std::string& modifierLogin,
	uint32_t ts,
	uint32_t offsInHistoryFile,
	uint32_t& prevTsModified,
	uint32_t& prevOffsModified)
{
	std::string textId;
	buf.GetString<uint8_t>(textId);
	std::string newBaseText;
	buf.GetString<uint16_t>(newBaseText);

	for (auto& f : db._folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslated::Ptr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			TextTranslated::Ptr tmpTextPtr = *result;
			f.timestampModified = ts;
			tmpTextPtr->baseText = newBaseText;
			tmpTextPtr->loginOfLastModifier = modifierLogin;
			prevTsModified = tmpTextPtr->timestampModified;
			tmpTextPtr->timestampModified = ts;
			prevOffsModified = tmpTextPtr->offsLastModified;
			tmpTextPtr->offsLastModified = offsInHistoryFile;
			return true;
		}
	}
	Log("ModifyDbChangeBaseText: text not found by id");
	return false;
}

//===============================================================================
//
//===============================================================================

bool  SClientMessagesMgr::ModifyDbAddAttributeToText(
	DeserializationBuffer& buf,
	TextsDatabase& db,
	const std::string& modifierLogin,
	uint32_t ts,
	uint32_t offsInHistoryFile,
	uint32_t& prevTsModified,
	uint32_t& prevOffsModified)
{
	std::string textId;
	buf.GetString<uint8_t>(textId);
	uint8_t attributeId = buf.GetUint<uint8_t>();
	uint8_t attributeDataType = buf.GetUint<uint8_t>();

	TextTranslated::Ptr tmpTextPtr;
	for (auto& f : db._folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslated::Ptr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			TextTranslated::Ptr tmpTextPtr = *result;
			f.timestampModified = ts;
			break;
		}
	}
	if (!tmpTextPtr) {
		Log("ModifyDbAddAttributeToText: text not found by id");
		return false;
	}

	tmpTextPtr->loginOfLastModifier = modifierLogin;
	prevTsModified = tmpTextPtr->timestampModified;
	tmpTextPtr->timestampModified = ts;
	prevOffsModified = tmpTextPtr->offsLastModified;
	tmpTextPtr->offsLastModified = offsInHistoryFile;

	auto result = std::find_if(std::begin(db._attributeProps), std::end(db._attributeProps), [&attributeId](const AttributeProperty& el) { return el.id == attributeId; });
	if (result == std::end(db._attributeProps)) {
		Log("ModifyDbAddAttributeToText: attribute not found by id");
		return false;
	}

	tmpTextPtr->attributes.emplace_back();
	auto& attributeInText = tmpTextPtr->attributes.back();
	attributeInText.id = attributeId;
	attributeInText.type = result->type;

	if (attributeDataType != result->type) {
		Log("ModifyDbAddAttributeToText: attributeDataType != result->type");
		return false;
	}

	switch (result->type)
	{
	case AttributeProperty::Translation_t:
	case AttributeProperty::CommonText_t:
		buf.GetString<uint16_t>(attributeInText.text);
		break;
	case AttributeProperty::Checkbox_t:
		attributeInText.flagState = buf.GetUint<uint8_t>();
		break;
	default:
		break;
	}
	return true;
}

//===============================================================================
//
//===============================================================================

bool  SClientMessagesMgr::ModifyDbDelAttributeFromText(
	DeserializationBuffer& buf,
	TextsDatabase& db,
	const std::string& modifierLogin,
	uint32_t ts,
	uint32_t offsInHistoryFile,
	uint32_t& prevTsModified,
	uint32_t& prevOffsModified)
{
	std::string textId;
	buf.GetString<uint8_t>(textId);
	uint8_t attributeId = buf.GetUint<uint8_t>();

	TextTranslated::Ptr tmpTextPtr;
	for (auto& f : db._folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslated::Ptr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			TextTranslated::Ptr tmpTextPtr = *result;
			f.timestampModified = ts;
			break;
		}
	}
	if (!tmpTextPtr) {
		Log("ModifyDbDelAttributeFromText: text not found by id");
		return false;
	}

	tmpTextPtr->loginOfLastModifier = modifierLogin;
	prevTsModified = tmpTextPtr->timestampModified;
	tmpTextPtr->timestampModified = ts;
	prevOffsModified = tmpTextPtr->offsLastModified;
	tmpTextPtr->offsLastModified = offsInHistoryFile;

	auto result = std::find_if(std::begin(tmpTextPtr->attributes), std::end(tmpTextPtr->attributes), [&attributeId](const AttributeInText& el) { return el.id == attributeId; });
	if (result == std::end(tmpTextPtr->attributes)) {
		Log("ModifyDbDelAttributeFromText: attribute not found by id");
		return false;
	}

	tmpTextPtr->attributes.erase(result);
	return true;
}

//===============================================================================
//
//===============================================================================

bool  SClientMessagesMgr::ModifyDbChangeAttributeInText(
	DeserializationBuffer& buf,
	TextsDatabase& db,
	const std::string& modifierLogin,
	uint32_t ts,
	uint32_t offsInHistoryFile,
	uint32_t& prevTsModified,
	uint32_t& prevOffsModified)
{
	std::string textId;
	buf.GetString<uint8_t>(textId);
	uint8_t attributeId = buf.GetUint<uint8_t>();
	uint8_t attributeDataType = buf.GetUint<uint8_t>();

	TextTranslated::Ptr tmpTextPtr;
	for (auto& f : db._folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslated::Ptr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			TextTranslated::Ptr tmpTextPtr = *result;
			f.timestampModified = ts;
			break;
		}
	}
	if (!tmpTextPtr) {
		Log("ModifyDbChangeAttributeInText: text not found by id");
		return false;
	}

	tmpTextPtr->loginOfLastModifier = modifierLogin;
	prevTsModified = tmpTextPtr->timestampModified;
	tmpTextPtr->timestampModified = ts;
	prevOffsModified = tmpTextPtr->offsLastModified;
	tmpTextPtr->offsLastModified = offsInHistoryFile;

	auto result = std::find_if(std::begin(tmpTextPtr->attributes), std::end(tmpTextPtr->attributes), [&attributeId](const AttributeInText& el) { return el.id == attributeId; });
	if (result == std::end(tmpTextPtr->attributes)) {
		Log("ModifyDbChangeAttributeInText: attribute not found by id");
		return false;
	}

	if (attributeDataType != result->type) {
		Log("ModifyDbAddAttributeToText: attributeDataType != result->type");
		return false;
	}

	switch (result->type)
	{
	case AttributeProperty::Translation_t:
	case AttributeProperty::CommonText_t:
		buf.GetString<uint16_t>(result->text);
		break;
	case AttributeProperty::Checkbox_t:
		result->flagState = buf.GetUint<uint8_t>();
		break;
	default:
		break;
	}
	return true;
}


//===============================================================================
//
//===============================================================================

SClientMessagesMgr::SClientMessagesMgr(STextsToolApp* app) : _app(app)
{
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::AddPacketToClients(SerializationBufferPtr& bufPtr, const std::string& srcDbName)
{
	for (auto client : _app->_clients) {
		if (client->_syncFinished && client->_dbName == srcDbName) {
			client->_msgsQueueOut.emplace_back(bufPtr);
		}
	}
}

//===============================================================================
//
//===============================================================================

bool SClientMessagesMgr::IfKeyALess(const uint8_t* p1, int size1, const uint8_t* p2, int size2)
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

//===============================================================================
//
//===============================================================================

SerializationBufferPtr SClientMessagesMgr::MakeSyncMessage(DeserializationBuffer& buf, TextsDatabase& db)
{
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

	auto buffer = std::make_shared<SerializationBuffer>();

	// Посылаем аттрибуты таблицы целиком
	buffer->Push((uint32_t)db._attributeProps.size());
	for (const auto& atribProp : db._attributeProps) {
		atribProp.SaveToBase(*buffer);
	}

	// Считать из пакета в clientFolders информацию о папках на клиенте
	std::vector<ClientFolder> clientFolders;
	uint32_t CltFoldersNum = buf.GetUint<uint32_t>();  // Количество папок на клиенте
	for (uint32_t folderIdx = 0; folderIdx < CltFoldersNum; ++folderIdx) {
		clientFolders.emplace_back(buf);
	}

	// Заполнить айдишники папок клиента, отсортировать их
	std::vector<uint32_t> clientFoldersIds;
	for (const auto& folder : clientFolders) {
		clientFoldersIds.emplace_back(folder.id);
	}
	std::sort(clientFoldersIds.begin(), clientFoldersIds.end());

	// Заполнить айдишники папок сервера, отсортировать их
	std::vector<uint32_t> serverFoldersIds;
	for (const auto& folder : db._folders) {
		serverFoldersIds.emplace_back(folder.id);
	}
	std::sort(serverFoldersIds.begin(), serverFoldersIds.end());

	// Найдём каталоги клиента подлежащие удалению (в клиентском списке есть, а в серверном их нет)
	std::vector<uint32_t> foldersToDelete;
	std::set_difference(clientFoldersIds.begin(), clientFoldersIds.end(), serverFoldersIds.begin(), serverFoldersIds.end(),
		std::inserter(foldersToDelete, foldersToDelete.begin()));

	// Добавим в пакет айдишники папок на удаление
	buffer->Push((uint32_t)foldersToDelete.size());
	for (auto id : foldersToDelete) {
		buffer->Push(id);
	}

	// Найдём каталоги сервера подлежащие созданию на клиенте (в серверном списке есть, а в клиентском их нет)
	std::vector<uint32_t> foldersToCreate;
	std::set_difference(serverFoldersIds.begin(), serverFoldersIds.end(), clientFoldersIds.begin(), clientFoldersIds.end(),
		std::inserter(foldersToCreate, foldersToCreate.begin()));

	// Добавим в пакет инфу о папках на создание
	buffer->Push((uint32_t)foldersToCreate.size());
	for (auto folderId : foldersToCreate) {
		auto& f = db._folders;
		auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
		if (result == std::end(f)) {
			ExitMsg("result == std::end(f)");
		}
		result->SaveToBase(*buffer);
	}

	// Найдём каталоги сервера, которые есть и на сервер и на клиенте. Чтобы проверить какие из них совпадают, а где есть отличия
	std::vector<uint32_t> foldersSameIds;
	std::set_intersection(serverFoldersIds.begin(), serverFoldersIds.end(), clientFoldersIds.begin(), clientFoldersIds.end(),
		std::inserter(foldersSameIds, foldersSameIds.begin()));

	// Сравниваем папки, которые есть и на клиенете и на сервере. Удалим айдишники папок с одинаковым временем модификации. 
	for (auto it = foldersSameIds.begin(); it != foldersSameIds.end();) {
		auto& f = db._folders;
		auto srvFoldrItr = std::find_if(std::begin(f), std::end(f), [folderId = *it](const Folder& el) { return el.id == folderId; });
		if (srvFoldrItr == std::end(f)) {
			ExitMsg("result == std::end(f)");
		}
		auto cltFoldrItr = std::find_if(std::begin(clientFolders), std::end(clientFolders), [folderId = *it](const ClientFolder& el) { return el.id == folderId; });
		if (cltFoldrItr == std::end(clientFolders)) {
			ExitMsg("cltFoldrItr == std::end(clientFolders)");
		}
		if (srvFoldrItr->timestampModified == cltFoldrItr->tsModified) {  // Если время модификации одинаковое на клиенте и сервере, то изменения по этой папке не нужно посылать на клиент
			it = foldersSameIds.erase(it);
		}
		else {
			++it;
		}
	}

	buffer->Push((uint32_t)foldersSameIds.size());

	// Обрабатываем папки, которые есть на клиенте и сервере, но время модификации у них разное

	for (auto folderId : foldersSameIds) {
		auto srvFoldrItr = std::find_if(std::begin(db._folders), std::end(db._folders), [folderId](const Folder& el) { return el.id == folderId; });
		auto cltFoldrItr = std::find_if(std::begin(clientFolders), std::end(clientFolders), [folderId](const ClientFolder& el) { return el.id == folderId; });

		// Начали записывать инфу об отличающейся папке

		buffer->Push(srvFoldrItr->id);
		buffer->PushStringWithoutZero<uint16_t>(srvFoldrItr->name);
		buffer->Push(srvFoldrItr->parentId);
		buffer->Push(srvFoldrItr->timestampModified);

		// Заполняем ключи серверных текстов и ссылки на них для быстрой сортировки по ключам

		std::vector<TextKey> textsKeys;       // Ключи серверных текстов текущей папки (для сортировки и разбиения на интевалы)
		std::vector<TextKey*> textsKeysRefs;  // Указатели на эти ключи для быстрой сортировки

		textsKeys.reserve(srvFoldrItr->texts.size());
		textsKeysRefs.reserve(srvFoldrItr->texts.size());

		for (const auto& t : srvFoldrItr->texts) {
			textsKeys.emplace_back();
			auto& textKey = textsKeys.back();
			textKey.textRef = &*t;
			textsKeysRefs.emplace_back(&textKey);
			MakeKey(t->timestampModified, t->id, textKey.key); // Склеивает ts модификации текста и текстовый айдишник текста в "ключ"
		}

		// Сортируем ссылки на заполненные ключи серверных текстов
		std::sort(textsKeysRefs.begin(), textsKeysRefs.end(), [](TextKey* el1, TextKey* el2) { return IfKeyALess(&el1->key[0], el1->key.size(), &el2->key[0], el2->key.size()); });

		// Заполнение интервалов отсортированных серверных текстов текущей папки
		std::vector<Interval> intervals;
		intervals.resize(cltFoldrItr->intervals.size());

		int cltKeyIdx = 0;
		int sum = 0;

		for (int i = 0; i < (int)textsKeysRefs.size(); ++i) {
			while (!IfKeyALess(&textsKeysRefs[i]->key[0], textsKeysRefs[i]->key.size(), &cltFoldrItr->keys[cltKeyIdx][0], cltFoldrItr->keys[cltKeyIdx].size())) {
				sum += intervals[cltKeyIdx].textsNum;
				++cltKeyIdx;
				intervals[cltKeyIdx].firstTextIdx = sum;
				if (cltKeyIdx == cltFoldrItr->keys.size()) {
					intervals[cltKeyIdx].textsNum = textsKeysRefs.size() - i;
					goto out;
				}
			}
			++(intervals[cltKeyIdx].textsNum);
		}
	out:    // Заполняем значения хэшей в интервалах (хэш интервала считается, как хэш ключей всех текстов интервала)
		for (auto& interval : intervals) {
			uint64_t hash = 0;
			for (uint32_t i = interval.firstTextIdx; i < (int)interval.firstTextIdx + interval.textsNum; ++i) {
				hash = AddHash(hash, textsKeysRefs[i]->key, i == interval.firstTextIdx);
			}
			interval.hash = hash;
		}

		// Сравниваем интервалы текущего каталога у сервера и клиента. Если текущий интервал отличается, то для него будут посланы все тексты

		uint32_t intervalsDifferNum = 0;  // Подсчитать количество отличающихся интервалов
		for (int i = 0; i < (int)intervals.size(); ++i) {
			if (intervals[i].textsNum != cltFoldrItr->intervals[i].textsInIntervalNum ||
				intervals[i].hash != cltFoldrItr->intervals[i].hashOfKeys) {
				++intervalsDifferNum;
			}
		}
		buffer->Push(intervalsDifferNum);
		for (uint32_t i = 0; i < intervals.size(); ++i) {
			if (intervals[i].textsNum != cltFoldrItr->intervals[i].textsInIntervalNum ||
				intervals[i].hash != cltFoldrItr->intervals[i].hashOfKeys) {
				buffer->Push(i);
				buffer->Push(intervals[i].textsNum);

				for (uint32_t i2 = intervals[i].firstTextIdx; i2 < intervals[i].firstTextIdx + intervals[i].textsNum; ++i2) {
					textsKeysRefs[i2]->textRef->SaveToBase(*buffer);
				}
			}
		}
	}
	return buffer;
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::MakeKey(uint32_t tsModified, const std::string& textId, std::vector<uint8_t>& result)
{
	result.resize(sizeof(uint32_t) + textId.size());
	uint8_t* p = &result[0];

	*(reinterpret_cast<uint32_t*>(p)) = tsModified;
	memcpy(p + sizeof(uint32_t), textId.c_str(), textId.size());
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::ConnectClient(const std::string& login, uint32_t sessionId)
{
	auto result = std::find_if(std::begin(_app->_clients), std::end(_app->_clients), [login](const SConnectedClient::Ptr& el) { return el->_login == login; });
	if (result != std::end(_app->_clients)) {
		_app->_clients.erase(result);
	}
	_app->_clients.emplace_back(std::make_shared<SConnectedClient>(login, sessionId));

	
//std::vector<uint8_t> pack1 = {20, 21, 22, 23};
//auto v1 = std::make_shared<SerializationBuffer>();
//v1->buffer = pack1;

//std::vector<uint8_t> pack2 = { 120, 121, 122 };
//auto v2 = std::make_shared<SerializationBuffer>();
//v2->buffer = pack2;

//std::vector<uint8_t> pack3 = { 220, 221, 222, 223, 224 };
//auto v3 = std::make_shared<SerializationBuffer>();
//v3->buffer = pack3;

//_app->_clients.back()->_msgsQueueOut.emplace_back(v1);
//_app->_clients.back()->_msgsQueueOut.emplace_back(v2);
//_app->_clients.back()->_msgsQueueOut.emplace_back(v3);
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::DisconnectClient(const std::string& login, uint32_t sessionId)
{
	auto result = std::find_if(std::begin(_app->_clients), std::end(_app->_clients), [login](const SConnectedClient::Ptr& el) { return el->_login == login; });
	if (result != std::end(_app->_clients)) {
		_app->_clients.erase(result);
	}
}

