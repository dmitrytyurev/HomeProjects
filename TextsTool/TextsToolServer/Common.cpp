#include "pch.h"

#include <ctime>
#include <iostream>
#include <experimental/filesystem>
#include <algorithm>
#include <iterator>

#include "Utils.h"
#include "Common.h"
#include "DbSerializer.h"

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
	_httpMgr.Update(dt);

	for (const auto& db : _dbs) {
		db->Update(dt);
	}
}

//===============================================================================
//
//===============================================================================

STextsToolApp::STextsToolApp(): 
	_messagesMgr(this), 
	_messagesRepaker(this), 
	_httpMgr(std::bind(&SClientMessagesMgr::ConnectClient, _messagesMgr, std::placeholders::_1, std::placeholders::_2),
		     std::bind(&SClientMessagesMgr::DisconnectClient, _messagesMgr, std::placeholders::_1, std::placeholders::_2))
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
	return nullptr;
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
	MutexLock lock(_app->_httpMgr._connections.mutex);

	for (auto& client : _app->_clients) {
		SConnectedClientLow* clLow = nullptr;
		for (auto& clientLow : _app->_httpMgr._connections.clients) {
			if (client->_login == clientLow->_login) {
				clLow = clientLow.get();
				break;
			}
		}
		if (!clLow) {
			continue;
		}

		if (client->_sessionId == clLow->_sessionId) {
			for (auto& buf : client->_msgsQueueOut) {
				clLow->_packetsQueueOut.PushPacket(buf->buffer, HttpPacket::Status::WAITING_FOR_PACKING);
			}
		}
		client->_msgsQueueOut.resize(0); // Очистим, даже если не скопировали (это был старый SConnectedClient ведь SConnectedClientLow уже подключился новый, так что из SConnectedClient не нужно было брать старые сообщения, они уже не актуальны)

		if (client->_sessionId == clLow->_sessionId) { // Если они не совпадут, значит создался новый SConnectedClientLow, а SConnectedClient ещё не успел пересоздаться, он сделает это на следующем Update и тогда мы снова придёт сюда и скопируем пакеты уже в нового SConnectedClient
			for (int iPckt = 0; iPckt < (int)clLow->_packetsQueueIn.size(); ++iPckt) {
				auto& packetPtr = clLow->_packetsQueueIn[iPckt];
				DeserializationBuffer buffer(packetPtr->_packetData); // !!! Неоптимально. Сделать возможность в DeserializationBuffer хранить указатель на вектор, а не копировать в него вектор целиком
				
std::string sstr;
for (auto val : buffer._buffer) {
sstr = sstr + std::to_string(val);
sstr = sstr + " ";
}
Log(sstr);
clLow->_packetsQueueIn.erase(clLow->_packetsQueueIn.begin() + iPckt);
--iPckt;




				
/*
				uint8_t pType = buffer.GetUint<uint8_t>();
				if (pType == PacketDataType::WholeMessages) { // В данном пакете один или несколько целых сообщений
					uint32_t messagesNum = buffer.GetUint<uint32_t>();
					for (int iMsg = 0; iMsg < (int)messagesNum; ++iMsg) {
						uint32_t messageSize = buffer.GetUint<uint32_t>();
						client->_msgsQueueIn.push_back(std::make_unique<DeserializationBuffer>(buffer.GetNextBytes(messageSize), messageSize));
						if (!buffer.IsEmpty()) {
							Log("!buffer.IsEmpty()");
						}
					}
					clLow->_packetsQueueIn.erase(clLow->_packetsQueueIn.begin() + iPckt);
					--iPckt;
				} 
				else if (pType == PacketDataType::PartOfMessage) { // В данном пакете первый фрагмент неполного сообщения
					uint32_t messageSize = buffer.GetUint<uint32_t>();
					uint32_t partMessageSize = buffer.GetUint<uint32_t>();
					uint32_t sum = partMessageSize;
					int iPckt2 = 0;
					for (iPckt2 = iPckt+1; iPckt2 < (int)clLow->_packetsQueueIn.size(); ++iPckt2) { // Просмотрим следующие пакеты, чтобы выяснить есть ли в них все фрагменты для сбора сообщения iPckt
						auto& packetPtr2 = clLow->_packetsQueueIn[iPckt2];
						DeserializationBuffer buffer2(packetPtr2->_packetData); // !!! Неоптимально. Сделать возможность в DeserializationBuffer хранить указатель на вектор, а не копировать в него вектор целиком
						uint8_t pType2 = buffer2.GetUint<uint8_t>();
						if (pType2 != PacketDataType::PartOfMessage) {
							Log("pType2 != PacketDataType::PartOfMessage"); // !!! Сделать корректный выход, если возможно
						}
						uint32_t messageSize2 = buffer2.GetUint<uint32_t>();
						if (messageSize != messageSize2) {
							ExitMsg("messageSize != messageSize2");
						}
						uint32_t partMessageSize2 = buffer2.GetUint<uint32_t>();
						sum += partMessageSize2;
						if (sum > messageSize) {
							ExitMsg("sum > messageSize");
						}
						if (sum == messageSize) {
							break; // Нашли последний пакет, содержащий данное сообщение. Выходим досрочно.
						}
					}
					if (iPckt2 < (int)clLow->_packetsQueueIn.size()) { // Вышли досрочно, значит найдены все пакеты с данным сообщением. Склеем его и удалим содержащие его пакеты
						int lastMsgPckt = iPckt2; // Последний пакет, содержащий данные текущего пакета
						client->_msgsQueueIn.emplace_back(std::make_unique<DeserializationBuffer>(buffer.GetNextBytes(partMessageSize), partMessageSize)); // Начнём склейку взяв данные сообщения из первого пакета
						DeserializationBuffer* pBuf = client->_msgsQueueIn.back().get();
						for (iPckt2 = iPckt+1; iPckt2 <= lastMsgPckt; ++iPckt2) { // Добавим данные из остальных пакетов (сразу будем удалять их)
							auto& packetPtr2 = clLow->_packetsQueueIn[iPckt2];
							DeserializationBuffer buffer2(packetPtr2->_packetData); // !!! Неоптимально. Сделать возможность в DeserializationBuffer хранить указатель на вектор, а не копировать в него вектор целиком
							uint8_t pType2 = buffer2.GetUint<uint8_t>();
							uint32_t messageSize2 = buffer2.GetUint<uint32_t>();
							uint32_t partMessageSize2 = buffer2.GetUint<uint32_t>();
							pBuf->AddBytes(buffer2.GetNextBytes(partMessageSize2), partMessageSize2);
							clLow->_packetsQueueIn.erase(clLow->_packetsQueueIn.begin() + iPckt2);
							--iPckt2;
							--lastMsgPckt;
						}
						clLow->_packetsQueueIn.erase(clLow->_packetsQueueIn.begin() + iPckt);
						--iPckt;
					}
					else { // Перебрали все оставшиеся пакеты, но всё сообщение склеить пока не получится. Выходим из разбора пакетов текущего клиента. 
						break;
					}
				}
				else {
					Log("Unknown packet type");
				}
*/


			}
		}
	}
}

//===============================================================================
//
//===============================================================================

SConnectedClient::SConnectedClient(const std::string& login, uint32_t sessionId) : _login(login), _sessionId(sessionId)
{
}
