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
	// !!! ������� ������ � ���
	std::cout << "Fatal: " << message << std::endl;
	exit(1);
}

//===============================================================================
//
//===============================================================================

void LogMsg(const std::string& message)
{
	// !!! ������� ������ � ���
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
	bufOut.Push(bufIn); // !!! ��� ��� �� ������ �� ��������� ���������� ������������� �������??
}

//===============================================================================
// ��������� ������ ������ ��������
//===============================================================================

void SClientMessagesMgr::SendToClients(const std::string& dbName, uint8_t ts, const DeserializationBuffer& buf)
{
	auto bufPtr = std::make_shared<SerializationBuffer>();       
	bufPtr->Push(ts);
	bufPtr->Push(buf);
	AddPacketToClients(bufPtr, dbName);
}


//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::Update(double dt)
{
	for (auto& client: _app->_clients) {
		// !!! ��������, ��� ���� ��������� ������� ���� � ���� ����, �� ��������� ������� ��������, � ���������� ������� ��������
		TextsDatabasePtr db = GetDbPtrByDbName(client->_dbName);
		for (const auto& buf : client->_msgsQueueIn) {
			uint8_t actionType = buf->GetUint<uint8_t>();
			uint32_t ts = GetTime();
			switch (actionType) {
			case DbSerializer::ActionCreateFolder:
			{
				db->_folders.emplace_back();                            // ��������� � ����
				Folder& folder = db->_folders.back();
				folder.CreateFromPacket(*buf, ts, db->_newFolderId++);
				folder.SaveToHistory(db, client->_login);               // ������ � ���� �������
				SerializationBufferPtr bufPtr = folder.SaveToPacket();  // ��������� ������ ������ �������� 
				AddPacketToClients(bufPtr, client->_dbName);
			}
			break;
			case DbSerializer::ActionDeleteFolder:
			{
				ModifyDbDeleteFolder(*buf, *db);
				SaveToHistory(db, client->_login, ts, *buf);      // ������ � ���� �������
				SendToClients(client->_dbName, ts, *buf);         // ��������� ������ ������ ��������
			}
			break;
			case DbSerializer::ActionChangeFolderParent:
			{
				ModifyDbChangeFolderParent(*buf, *db, ts);
				SaveToHistory(db, client->_login, ts, *buf);      // ������ � ���� �������
				SendToClients(client->_dbName, ts, *buf);         // ��������� ������ ������ ��������
			}
			break;
			case DbSerializer::ActionRenameFolder:
			{
				ModifyDbRenameFolder(*buf, *db, ts);              // ��������� � ����
				SaveToHistory(db, client->_login, ts, *buf);      // ������ � ���� �������
				SendToClients(client->_dbName, ts, *buf);         // ��������� ������ ������ ��������
			}
			break;			
			case DbSerializer::ActionCreateAttribute:
			{
				db->_attributeProps.emplace_back();                 // ��������� � ����
				AttributeProperty& ap = db->_attributeProps.back();
				ap.CreateFromPacket(*buf, ts, db->_newAttributeId++);
				ap.SaveToHistory(db, client->_login);               // ������ � ���� �������
				SerializationBufferPtr bufPtr = ap.SaveToPacket();  // ��������� ������ ������ �������� 
				AddPacketToClients(bufPtr, client->_dbName);
				db->_newAttributeId = db->_attributeProps.back().id + 1;
			}
			break;
			case DbSerializer::ActionDeleteAttribute:
			{
				ModifyDbDeleteAttribute(*buf, *db, ts);           // ��������� � ����
				SaveToHistory(db, client->_login, ts, *buf);      // ������ � ���� �������
				SendToClients(client->_dbName, ts, *buf);         // ��������� ������ ������ ��������			
			}
			break;
			case DbSerializer::ActionRenameAttribute:
			{
				ModifyDbRenameAttribute(*buf, *db);               // ��������� � ����
				SaveToHistory(db, client->_login, ts, *buf);      // ������ � ���� �������
				SendToClients(client->_dbName, ts, *buf);         // ��������� ������ ������ ��������			
			}
			break;
			case DbSerializer::ActionChangeAttributeVis:
			{
				ModifyDbChangeAttributeVis(*buf, *db);            // ��������� � ����
				SaveToHistory(db, client->_login, ts, *buf);      // ������ � ���� �������
				SendToClients(client->_dbName, ts, *buf);         // ��������� ������ ������ ��������			
			}
			break;
			case DbSerializer::ActionCreateText:
			{
				uint32_t folderId = buf->GetUint<uint32_t>();     // ��������� � ����
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
					tt.SaveToHistory(db, folderId);                     // ������ � ���� �������
					SerializationBufferPtr bufPtr = tt.SaveToPacket(folderId);  // ��������� ������ ������ �������� 
					AddPacketToClients(bufPtr, client->_dbName);
				}
				else {
					LogMsg("SClientMessagesMgr::Update: ActionCreateText: folder not found");
				}
			}
			break;
			default:
				ExitMsg("Wrong actionType");
			}
		}
		client->_msgsQueueIn.resize(0); // �� ���������� clear, ����� ������������� ���������� ���������
	}
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::ModifyDbDeleteFolder(DeserializationBuffer& buf, TextsDatabase& db)
{
	uint32_t folderId = buf.GetUint<uint32_t>();
	auto& f = db._folders;                                 // ��������� � ����
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
	auto& f = db._folders;                                  // ��������� � ����
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
	uint8_t visPosOfDeletedAttr = 0;                   // ��������� � ����
	auto& ap = db._attributeProps;
	auto result = std::find_if(std::begin(ap), std::end(ap), [attributeId](const AttributeProperty& el) { return el.id == attributeId; });
	if (result != std::end(ap)) {
		visPosOfDeletedAttr = result->visiblePosition;
		ap.erase(result);
	}
	else {
		LogMsg("SClientMessagesMgr::Update::ActionDeleteAttribute: attribute id not found");
	}
	for (auto it = ap.begin(); it != ap.end(); ) {  // �������� ������� ���� ��������� ������ �� ����������
		if (it->visiblePosition > visPosOfDeletedAttr) {
			--(it->visiblePosition);
		}
	}
	for (auto& folder : db._folders) {  // ������� �������� � ����� id �� ���� �������
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

void SClientMessagesMgr::ModifyDbDeleteText(DeserializationBuffer& buf, TextsDatabase& db)
{
	std::string textId;
	buf.GetString<uint8_t>(textId);
	for (auto& f: db._folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslated::Ptr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			f.texts.erase(result);
			return;
		}
	}
	ExitMsg("ModifyDbDeleteText: text id not found");
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


