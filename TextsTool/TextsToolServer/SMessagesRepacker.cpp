#include "pch.h"
#include <memory>

#include "SMessagesRepacker.h"
#include "Common.h"
#include "Utils.h"
#include "../SharedSrc/Shared.h"

//===============================================================================

SMessagesRepacker::SMessagesRepacker(STextsToolApp* app) : _app(app)
{

}

//===============================================================================

void SMessagesRepacker::RepackMessagesOutToPackets(std::shared_ptr<SConnectedClient>& client, SConnectedClientLow* clLow)
{
	if (client->_sessionId != clLow->_sessionId) {
		// Очистим, даже если не скопировали (это был старый SConnectedClient ведь SConnectedClientLow уже подключился новый, 
		// так что из SConnectedClient не нужно было брать старые сообщения, они уже не актуальны)
		client->_msgsQueueOut.resize(0);
	}

	const int packetMaxSize = HTTP_BUF_SIZE - 200; // Максимальный размер данных сообщения в пакете. Берётся меньше размера буфера получения HTTP-запроса, потому, что к данным сообщения ещё добавляются данные репакера и данные HTTP-менеджера

	auto& v = client->_msgsQueueOut;

	while (v.size()) {
		if (v[0]->buffer.size() > packetMaxSize) {
			// Сообщение превышает максимальный размер пакета. Разбиваем его на несколько пакетов
			int n = (v[0]->buffer.size() + packetMaxSize - 1) / packetMaxSize; // Число пакетов, на которое разобьём сообщение
			int offs = 0;
			for (int i = 0; i < n; ++i) {
				int curSize = (i == n - 1 ? v[0]->buffer.size() - (n - 1) * packetMaxSize : packetMaxSize);
				SerializationBuffer sbuf;
				sbuf.Push((uint8_t)PacketDataType::PartOfMessage);
				sbuf.Push((uint32_t)v[0]->buffer.size()); // Длина полного сообщений
				sbuf.Push((uint32_t)curSize);  // Длина текущего куска сообщения
				sbuf.PushBytes(v[0]->buffer.data() + offs, curSize);
				offs += curSize;
				clLow->_packetsQueueOut.PushPacket(sbuf.buffer, HttpPacket::Status::WAITING_FOR_PACKING);
			}
			v.erase(v.begin());
		}
		else {
			// Объединяем по несколько сообщений в пакет (от 1 до N сообщений)
			int n = 0;
			int sum = 0;
			// Считаем число сообщений n, который поместятся в один пакет
			while (sum + v[n]->buffer.size() + sizeof(uint32_t) <= packetMaxSize) {  // Добавляется sizeof(uint32_t) потому, что к каждому сообщению
				sum += v[n]->buffer.size() + sizeof(uint32_t);                       //                                                              в пакете допишется его длина - это добавочные расходы длины пакета
				if (++n == v.size()) {
					break;
				}
			};
			if (n == 0) {
				n = 1; // Если длина данных пакета в интервале [packetMaxSize-sizeof(uint32_t)..packetMaxSize], то получим n == 0
			}
			// Заносим n очередных сообщений в один пакет
			SerializationBuffer sbuf;
			sbuf.Push(PacketDataType::WholeMessages);
			sbuf.Push((uint32_t)n); // Количество целых сообщений в пакете
			for (int i = 0; i < n; ++i) {
				sbuf.Push((uint32_t)v[0]->buffer.size());
				sbuf.PushBytes(v[0]->buffer.data(), v[0]->buffer.size());
				v.erase(v.begin());
			}
			clLow->_packetsQueueOut.PushPacket(sbuf.buffer, HttpPacket::Status::WAITING_FOR_PACKING);
		}
	}
}

//===============================================================================

void SMessagesRepacker::RepackPacketsInToMessages(std::shared_ptr<SConnectedClient>& client, SConnectedClientLow* clLow)
{
	if (client->_sessionId != clLow->_sessionId) {
		// Если они не совпадут, значит создался новый SConnectedClientLow, а SConnectedClient ещё не успел пересоздаться, 
		// он сделает это на следующем Update и тогда мы снова придём сюда и скопируем пакеты уже в нового SConnectedClient
		return;
	}

	for (int iPckt = 0; iPckt < (int)clLow->_packetsQueueIn.size(); ++iPckt) {
		auto& packetPtr = clLow->_packetsQueueIn[iPckt];
		DeserializationBuffer buffer(packetPtr->_packetData); // !!! Неоптимально. Сделать возможность в DeserializationBuffer хранить указатель на вектор, а не копировать в него вектор целиком
		Utils::LogBuf(buffer._buffer);
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
			for (iPckt2 = iPckt + 1; iPckt2 < (int)clLow->_packetsQueueIn.size(); ++iPckt2) { // Просмотрим следующие пакеты, чтобы выяснить есть ли в них все фрагменты для сбора сообщения iPckt
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
				for (iPckt2 = iPckt + 1; iPckt2 <= lastMsgPckt; ++iPckt2) { // Добавим данные из остальных пакетов (сразу будем удалять их)
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
	}
}

//===============================================================================

void SMessagesRepacker::Update(double dt)
{
	Utils::MutexLock lock(_app->_httpMgr._connections.mutex);

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
		RepackMessagesOutToPackets(client, clLow);
		RepackPacketsInToMessages(client, clLow);
	}
}

