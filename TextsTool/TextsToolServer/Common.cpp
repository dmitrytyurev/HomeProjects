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
	bufOut.Push(bufIn); // !!! Как это по ссылке на константу вызывается неконстантная функция??
}

//===============================================================================
// Разослать пакеты другим клиентам
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
		// !!! Вероятно, тут надо проверить наличие базы и если нету, то запустить фоновую загрузку, а выполнение запроса отложить
		TextsDatabasePtr db = GetDbPtrByDbName(client->_dbName);
		for (const auto& buf : client->_msgsQueueIn) {
			uint8_t actionType = buf->GetUint<uint8_t>();
			switch (actionType) {
			case DbSerializer::ActionCreateFolder:
			{
				db->_folders.emplace_back();                                 // Изменения в базе
				Folder& folder = db->_folders.back();
				uint32_t ts = GetTime();
				folder.CreateFromPacket(*buf, ts);
				folder.id = db->_newFolderId++;
				folder.SaveToHistory(db, client->_login); // Запись в файл истории
				SerializationBufferPtr bufPtr = folder.SaveToPacket();        // Разослать пакеты другим клиентам 
				AddPacketToClients(bufPtr, client->_dbName);
			}
			break;
			case DbSerializer::ActionDeleteFolder:
			{
				uint32_t folderId = buf->GetUint<uint32_t>();
				auto& f = db->_folders;                                      // Изменения в базе
				auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
				if (result != std::end(f)) {
					f.erase(result);
				} else {
					LogMsg("SClientMessagesMgr::Update:DbSerializer::ActionDeleteFolder: folder not found");
				}
				uint32_t ts = GetTime();
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				SendToClients(client->_dbName, ts, *buf);         // Разослать пакеты другим клиентам
			}
			break;
			case DbSerializer::ActionChangeFolderParent:
			{
				uint32_t ts = GetTime();
				uint32_t folderId = buf->GetUint<uint32_t>();
				uint32_t newParentFolderId = buf->GetUint<uint32_t>();
				auto& f = db->_folders;                                      // Изменения в базе
				auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
				if (result != std::end(f)) {
					result->parentId = newParentFolderId;
					result->timestampModified = ts;
				} else {
					LogMsg("SClientMessagesMgr::Update:DbSerializer::ActionChangeFolderParent: folder not found");
				}
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				SendToClients(client->_dbName, ts, *buf);         // Разослать пакеты другим клиентам
			}
			break;
			case DbSerializer::ActionRenameFolder:
			{
				uint32_t ts = GetTime();
				uint32_t folderId = buf->GetUint<uint32_t>();
				std::string newFolderName;
				buf->GetString<uint8_t>(newFolderName);
				auto& f = db->_folders;                                      // Изменения в базе
				auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
				if (result != std::end(f)) {
					result->name = newFolderName;
					result->timestampModified = ts;
				}
				else {
					LogMsg("SClientMessagesMgr::Update:DbSerializer::ActionRenameFolder: folder not found");
				}
				SaveToHistory(db, client->_login, ts, *buf);      // Запись в файл истории
				SendToClients(client->_dbName, ts, *buf);         // Разослать пакеты другим клиентам
			}
			break;			
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
			MutexLock lock(clLow->packetsQueueOut.mutex);
			for (auto& buf : client->_msgsQueueOut) {
				clLow->packetsQueueOut.PushPacket(buf->buffer);
			}
			client->_msgsQueueOut.resize(0);
		}
		{
			MutexLock lock(clLow->packetsQueueIn.mutex);
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


