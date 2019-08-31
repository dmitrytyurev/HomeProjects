#include "pch.h"

#include <iostream>
#include <ctime>
#include <iostream>
#include <experimental/filesystem>

#include "Common.h"
#include "DbSerializer.h"

//===============================================================================
//
//===============================================================================

void ExitMsg(const std::string& message)
{
	// !!! Сделать запись в лог
	std::cout << "Fatal: " << message << std::endl;
	exit(1);
}

//===============================================================================
//
//===============================================================================

void LogMsg(const std::string& message)
{
	// !!! Сделать запись в лог
	std::cout << message << std::endl;
}

//===============================================================================
//
//===============================================================================

uint32_t GetTime()
{
	return static_cast<uint32_t>(std::time(0));

}

//===============================================================================
//
//===============================================================================

void PeriodUpdater::SetPeriod(double period)
{
	_period = period;
}

//===============================================================================
//
//===============================================================================

bool PeriodUpdater::IsTimeToProceed(double dt)
{
	_timeToProcess -= dt;
	bool isTimeToProceed = (_timeToProcess <= 0);

	if (isTimeToProceed) {
		_timeToProcess += _period;
	}
	return isTimeToProceed;
}




//===============================================================================
//
//===============================================================================

void STextsToolApp::Update(double dt)
{
	_messagesMgr.Update(dt);
	_messagesRepaker.Update(dt);

	for (const auto& db : _dbs) {
		db->Update(dt);
	}
}

//===============================================================================
//
//===============================================================================

STextsToolApp::STextsToolApp(): _messagesMgr(this), _messagesRepaker(this)
{
}

//===============================================================================
//
//===============================================================================

TextsDatabasePtr SClientMessagesMgr::GetDbPtrByDbName(const std::string& dbName)
{
	for (const auto& db : _app->_dbs) {
		if (dbName == db->_dbName) {
			return db;
		}
	}
	ExitMsg("Database not found: " + dbName);  
	return nullptr;
}

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
	for (auto& client: _app->_clients) {
		// !!! Вероятно, тут надо проверить наличие базы и если нету, то запустить фоновую загрузку, а выполнение запроса отложить
		TextsDatabasePtr db = GetDbPtrByDbName(client->_dbName);
		for (const auto& buf : client->_msgsQueueIn) {
			uint8_t actionType = buf->GetUint<uint8_t>();
			uint32_t ts = GetTime();
			switch (actionType) {
			case DbSerializer::ActionCreateFolder:
			{
				db->_folders.emplace_back();                            // Изменения в базе
				Folder& folder = db->_folders.back();
				folder.CreateFromPacket(*buf, ts, db->_newFolderId++);
				folder.SaveToHistory(db, client->_login);               // Запись в файл истории
				SerializationBufferPtr bufPtr = folder.SaveToPacket();  // Разослать пакеты другим клиентам 
				AddPacketToClients(bufPtr, client->_dbName);
			}
			break;
			case DbSerializer::ActionDeleteFolder:
			{
				ModifyDbDeleteFolder(*buf, *db);
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				SendToClients(client->_dbName, ts, *buf, "");         // Разослать пакеты другим клиентам
			}
			break;
			case DbSerializer::ActionChangeFolderParent:
			{
				ModifyDbChangeFolderParent(*buf, *db, ts);
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				SendToClients(client->_dbName, ts, *buf, "");         // Разослать пакеты другим клиентам
			}
			break;
			case DbSerializer::ActionRenameFolder:
			{
				ModifyDbRenameFolder(*buf, *db, ts);              // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				SendToClients(client->_dbName, ts, *buf, "");         // Разослать пакеты другим клиентам
			}
			break;			
			case DbSerializer::ActionCreateAttribute:
			{
				db->_attributeProps.emplace_back();                 // Изменения в базе
				AttributeProperty& ap = db->_attributeProps.back();
				ap.CreateFromPacket(*buf, ts, db->_newAttributeId++);
				ap.SaveToHistory(db, client->_login);               // Запись в файл истории
				SerializationBufferPtr bufPtr = ap.SaveToPacket();  // Разослать пакеты другим клиентам 
				AddPacketToClients(bufPtr, client->_dbName);
				db->_newAttributeId = db->_attributeProps.back().id + 1;
			}
			break;
			case DbSerializer::ActionDeleteAttribute:
			{
				ModifyDbDeleteAttribute(*buf, *db, ts);           // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				SendToClients(client->_dbName, ts, *buf, "");         // Разослать пакеты другим клиентам			
			}
			break;
			case DbSerializer::ActionRenameAttribute:
			{
				ModifyDbRenameAttribute(*buf, *db);               // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				SendToClients(client->_dbName, ts, *buf, "");         // Разослать пакеты другим клиентам			
			}
			break;
			case DbSerializer::ActionChangeAttributeVis:
			{
				ModifyDbChangeAttributeVis(*buf, *db);            // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				SendToClients(client->_dbName, ts, *buf, "");         // Разослать пакеты другим клиентам			
			}
			break;
			case DbSerializer::ActionCreateText:
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
					SerializationBufferPtr bufPtr = tt.SaveToPacket(folderId);  // Разослать пакеты другим клиентам 
					AddPacketToClients(bufPtr, client->_dbName);
				}
				else {
					LogMsg("SClientMessagesMgr::Update: ActionCreateText: folder not found");
				}
			}
			break;
			case DbSerializer::ActionDeleteText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				ModifyDbDeleteText(*buf, *db, prevTsModified, prevOffsModified);                 // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);   // Запись в файл истории
				db->GetHistoryBuffer().Push(prevTsModified);
				db->GetHistoryBuffer().Push(prevOffsModified);
				SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
			}
			break;
			case DbSerializer::ActionMoveTextToFolder:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				ModifyDbMoveTextToFolder(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);               // Запись в файл истории
				db->GetHistoryBuffer().Push(prevTsModified);
				db->GetHistoryBuffer().Push(prevOffsModified);
				SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
			}
			break;
			case DbSerializer::ActionChangeBaseText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				ModifyDbChangeBaseText(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				SaveToHistory(db, client->_login, ts, *buf);               // Запись в файл истории
				db->GetHistoryBuffer().Push(prevTsModified);
				db->GetHistoryBuffer().Push(prevOffsModified);
				SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
			}
			break;
			case DbSerializer::ActionAddAttributeToText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				uint32_t keepOffset = buf->offset;
				ModifyDbAddAttributeToText(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				buf->offset = keepOffset; // Восстнавливаем буфер на состояние "прочитан только тип операции"
				auto& historyBuf = db->GetHistoryBuffer();                     // Запись в файл истории
				historyBuf.PushStringWithoutZero<uint8_t>(client->_login);
				historyBuf.Push(ts);
				historyBuf.Push(actionType);
				historyBuf.Push(prevTsModified);
				historyBuf.Push(prevOffsModified);
				historyBuf.Push(*buf, false); // Заберём только непрочитанные данные (всё кроме типа операции)
				SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
			}
			break;
			case DbSerializer::ActionDelAttributeFromText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				uint32_t keepOffset = buf->offset;
				ModifyDbDelAttributeFromText(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				buf->offset = keepOffset; // Восстнавливаем буфер на состояние "прочитан только тип операции"
				auto& historyBuf = db->GetHistoryBuffer();                     // Запись в файл истории
				historyBuf.PushStringWithoutZero<uint8_t>(client->_login);
				historyBuf.Push(ts);
				historyBuf.Push(actionType);
				historyBuf.Push(prevTsModified);
				historyBuf.Push(prevOffsModified);
				historyBuf.Push(*buf, false); // Заберём только непрочитанные данные (всё кроме типа операции)
				SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
			}
			break;
			case DbSerializer::ActionChangeAttributeInText:
			{
				uint32_t prevTsModified = 0;
				uint32_t prevOffsModified = 0;
				uint32_t keepOffset = buf->offset;
				ModifyDbChangeAttributeInText(*buf, *db, client->_login, ts, db->GetCurrentPosInHistoryFile(), prevTsModified, prevOffsModified); // Изменения в базе
				buf->offset = keepOffset; // Восстнавливаем буфер на состояние "прочитан только тип операции"
				auto& historyBuf = db->GetHistoryBuffer();                     // Запись в файл истории
				historyBuf.PushStringWithoutZero<uint8_t>(client->_login);
				historyBuf.Push(ts);
				historyBuf.Push(actionType);
				historyBuf.Push(prevTsModified);
				historyBuf.Push(prevOffsModified);
				historyBuf.Push(*buf, false); // Заберём только непрочитанные данные (всё кроме типа операции)
				SendToClients(client->_dbName, ts, *buf, client->_login);  // Разослать пакеты другим клиентам			
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

void SClientMessagesMgr::ModifyDbDeleteFolder(DeserializationBuffer& buf, TextsDatabase& db)
{
	uint32_t folderId = buf.GetUint<uint32_t>();
	auto& f = db._folders;                                 // Изменения в базе
	auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
	if (result != std::end(f)) {
		f.erase(result);
	}
	else {
		LogMsg("SClientMessagesMgr::Update::ActionDeleteFolder: folder not found");
	}
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::ModifyDbChangeFolderParent(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts)
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
		LogMsg("SClientMessagesMgr::Update::ActionChangeFolderParent: folder not found");
	}
}

//===============================================================================
//
//===============================================================================


void SClientMessagesMgr::ModifyDbRenameFolder(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts)
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
		LogMsg("SClientMessagesMgr::Update::ActionRenameFolder: folder not found");
	}
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::ModifyDbDeleteAttribute(DeserializationBuffer& buf, TextsDatabase& db, uint32_t ts)
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
		LogMsg("SClientMessagesMgr::Update::ActionDeleteAttribute: attribute id not found");
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
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::ModifyDbRenameAttribute(DeserializationBuffer& buf, TextsDatabase& db)
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
		ExitMsg("ModifyDbRenameAttribute: attribute id not found");
	}
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::ModifyDbChangeAttributeVis(DeserializationBuffer& buf, TextsDatabase& db)
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
		ExitMsg("ModifyDbChangeAttributeVis: attribute id not found");
	}
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::ModifyDbDeleteText(DeserializationBuffer& buf, TextsDatabase& db, uint32_t& prevTsModified, uint32_t& prevOffsModified)
{
	std::string textId;
	buf.GetString<uint8_t>(textId);
	for (auto& f: db._folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslated::Ptr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			prevTsModified = (*result)->timestampModified;
			prevOffsModified = (*result)->offsLastModified;
			f.texts.erase(result);
			return;
		}
	}
	ExitMsg("ModifyDbDeleteText: text not found by id");
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::ModifyDbMoveTextToFolder(
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
					return;
				}
			}
			ExitMsg("ModifyDbMoveTextToFolder: folder not found");
			return;
		}
	}
	ExitMsg("ModifyDbMoveTextToFolder: text not found by id");
}

//===============================================================================
//
//===============================================================================

void  SClientMessagesMgr::ModifyDbChangeBaseText(
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
			return;
		}
	}
	ExitMsg("ModifyDbChangeBaseText: text not found by id");
}

//===============================================================================
//
//===============================================================================

void  SClientMessagesMgr::ModifyDbAddAttributeToText(
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
		ExitMsg("ModifyDbAddAttributeToText: text not found by id");
	}

	tmpTextPtr->loginOfLastModifier = modifierLogin;
	prevTsModified = tmpTextPtr->timestampModified;
	tmpTextPtr->timestampModified = ts;
	prevOffsModified = tmpTextPtr->offsLastModified;
	tmpTextPtr->offsLastModified = offsInHistoryFile;

	auto result = std::find_if(std::begin(db._attributeProps), std::end(db._attributeProps), [&attributeId](const AttributeProperty& el) { return el.id == attributeId; });
	if (result == std::end(db._attributeProps)) {
		ExitMsg("ModifyDbAddAttributeToText: attribute not found by id");
	}

	tmpTextPtr->attributes.emplace_back();
	auto& attributeInText = tmpTextPtr->attributes.back();
	attributeInText.id = attributeId;
	attributeInText.type = result->type;

	if (attributeDataType != result->type) {
		ExitMsg("ModifyDbAddAttributeToText: attributeDataType != result->type");
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
}

//===============================================================================
//
//===============================================================================

void  SClientMessagesMgr::ModifyDbDelAttributeFromText(
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
		ExitMsg("ModifyDbDelAttributeFromText: text not found by id");
	}

	tmpTextPtr->loginOfLastModifier = modifierLogin;
	prevTsModified = tmpTextPtr->timestampModified;
	tmpTextPtr->timestampModified = ts;
	prevOffsModified = tmpTextPtr->offsLastModified;
	tmpTextPtr->offsLastModified = offsInHistoryFile;

	auto result = std::find_if(std::begin(tmpTextPtr->attributes), std::end(tmpTextPtr->attributes), [&attributeId](const AttributeInText& el) { return el.id == attributeId; });
	if (result == std::end(tmpTextPtr->attributes)) {
		ExitMsg("ModifyDbDelAttributeFromText: attribute not found by id");
	}

	tmpTextPtr->attributes.erase(result);
}

//===============================================================================
//
//===============================================================================

void  SClientMessagesMgr::ModifyDbChangeAttributeInText(
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
		ExitMsg("ModifyDbChangeAttributeInText: text not found by id");
	}

	tmpTextPtr->loginOfLastModifier = modifierLogin;
	prevTsModified = tmpTextPtr->timestampModified;
	tmpTextPtr->timestampModified = ts;
	prevOffsModified = tmpTextPtr->offsLastModified;
	tmpTextPtr->offsLastModified = offsInHistoryFile;

	auto result = std::find_if(std::begin(tmpTextPtr->attributes), std::end(tmpTextPtr->attributes), [&attributeId](const AttributeInText& el) { return el.id == attributeId; });
	if (result == std::end(tmpTextPtr->attributes)) {
		ExitMsg("ModifyDbChangeAttributeInText: attribute not found by id");
	}

	if (attributeDataType != result->type) {
		ExitMsg("ModifyDbAddAttributeToText: attributeDataType != result->type");
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
}


//===============================================================================
//
//===============================================================================

SClientMessagesMgr::SClientMessagesMgr(STextsToolApp* app): _app(app)
{
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::AddPacketToClients(SerializationBufferPtr& bufPtr, const std::string& srcDbName)
{
	for (auto client: _app->_clients) {
		if (client->_dbName == srcDbName) {
			client->_msgsQueueOut.emplace_back(bufPtr);
		}
	}
}

//===============================================================================
//
//===============================================================================

SMessagesRepaker::SMessagesRepaker(STextsToolApp* app): _app(app)
{
}

//===============================================================================
//
//===============================================================================

void SMessagesRepaker::Update(double dt)
{
	MutexLock lock(_app->_httpManager._mtClients.mutex);
	for (auto& client : _app->_clients) {
		SConnectedClientLow* clLow = nullptr;
		for (auto& clientLow : _app->_httpManager._mtClients.clients) {
			if (client->_login == clientLow->login) {
				clLow = clientLow.get();
				break;
			}
		}
		if (!clLow) {
			continue;
		}
		{
			MutexLock lock(_app->_httpManager._mtClients.mutex);
			for (auto& buf : client->_msgsQueueOut) {
				clLow->packetsQueueOut.PushPacket(buf->buffer);
			}
			client->_msgsQueueOut.resize(0);
			for (auto& packetPtr : clLow->packetsQueueIn.queue) {
				client->_msgsQueueIn.push_back(std::make_unique<DeserializationBuffer>(packetPtr->_packetData));
			}
			clLow->packetsQueueIn.queue.resize(0);
		}
	}
}

//===============================================================================
//
//===============================================================================

HttpPacket::HttpPacket(uint32_t packetIndex, std::vector<uint8_t>& packetData): _packetIndex(packetIndex), _packetData(packetData)
{
}

//===============================================================================
//
//===============================================================================

void MTQueueOut::PushPacket(std::vector<uint8_t>& data)
{
	queue.push(std::make_unique<HttpPacket>(lastSentPacketN++, data));
}


