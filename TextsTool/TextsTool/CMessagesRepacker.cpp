#include <memory>

#include "CMessagesRepacker.h"
#include "../SharedSrc/Shared.h"

void Log(const std::string& str);

//===============================================================================

void Repacker::RepackMessagesOutToPackets(std::vector<SerializationBufferPtr>& msgsQueueOut, CHttpManager& httpManager)
{
	const int packetMaxSize = HTTP_BUF_SIZE - 200; // Максимальный размер данных сообщения в пакете. Берётся меньше размера буфера получения HTTP-запроса, потому, что к данным сообщения ещё добавляются данные репакера и данные HTTP-менеджера

    auto& v = msgsQueueOut;

	while (v.size()) {
		if (v[0]->buffer.size() > packetMaxSize) {
			// Сообщение превышает максимальный размер пакета. Разбиваем его на несколько пакетов
			int n = (v[0]->buffer.size() + packetMaxSize - 1) / packetMaxSize; // Число пакетов, на которое разобьём сообщение
			int offs = 0;
			for (int i = 0; i < n; ++i) {
                int curSize =(i == n - 1 ? (int)v[0]->buffer.size() - (n - 1) * packetMaxSize : packetMaxSize);
				SerializationBuffer sbuf;
				sbuf.PushUint8(PacketDataType::PartOfMessage);
				sbuf.PushUint32(v[0]->buffer.size()); // Длина полного сообщений
				sbuf.PushUint32(curSize);  // Длина текущего куска сообщения
				sbuf.PushBytes(v[0]->buffer.data() + offs, curSize);
				offs += curSize;
                httpManager.PutPacketToSendQueue(sbuf.buffer);
			}
			v.erase(v.begin());
		}
		else {
			// Объединяем по несколько сообщений в пакет (от 1 до N сообщений)
			int n = 0;
			int sum = 0;
			// Считаем число сообщений n, который поместятся в один пакет
            while (sum + v[n]->buffer.size() + sizeof(uint32_t) <= packetMaxSize) {  // Добавляется sizeof(uint32_t) потому, что к каждому сообщению
                sum += (int)v[n]->buffer.size() + (int)sizeof(uint32_t);                       // в пакете допишется его длина - это добавочные расходы длины пакета
                if (++n == (int)v.size()) {
					break;
				}
			};
			if (n == 0) {
				n = 1; // Если длина данных пакета в интервале [packetMaxSize-sizeof(uint32_t)..packetMaxSize], то получим n == 0
			}
			// Заносим n очередных сообщений в один пакет
			SerializationBuffer sbuf;
			sbuf.PushUint8(PacketDataType::WholeMessages);
			sbuf.PushUint32(n); // Количество целых сообщений в пакете
			for (int i = 0; i < n; ++i) {
				sbuf.PushUint32(v[0]->buffer.size());
                sbuf.PushBytes(v[0]->buffer.data(), (int)v[0]->buffer.size());
				v.erase(v.begin());
			}
            httpManager.PutPacketToSendQueue(sbuf.buffer);
		}
	}
}

//===============================================================================

void Repacker::RepackPacketsInToMessages(CHttpManager& httpManager, std::vector<DeserializationBufferPtr>& msgsQueueIn)
{
    for (int iPckt = 0; iPckt < (int)httpManager._packetsIn.size(); ++iPckt) {
        auto& packetPtr = httpManager._packetsIn[iPckt];
		DeserializationBuffer buffer(packetPtr->_packetData); // !!! Неоптимально. Сделать возможность в DeserializationBuffer хранить указатель на вектор, а не копировать в него вектор целиком
		uint8_t pType = buffer.GetUint8();
		if (pType == PacketDataType::WholeMessages) { // В данном пакете один или несколько целых сообщений
			uint32_t messagesNum = buffer.GetUint32();
			for (int iMsg = 0; iMsg < (int)messagesNum; ++iMsg) {
				uint32_t messageSize = buffer.GetUint32();
                msgsQueueIn.push_back(std::make_unique<DeserializationBuffer>(buffer.GetNextBytes(messageSize), messageSize));
			}
			if (!buffer.IsEmpty()) {
				Log("!buffer.IsEmpty()");
			}
			httpManager._packetsIn.erase(httpManager._packetsIn.begin() + iPckt);
			--iPckt;
		}
		else if (pType == PacketDataType::PartOfMessage) { // В данном пакете первый фрагмент неполного сообщения
			uint32_t messageSize = buffer.GetUint32();
			uint32_t partMessageSize = buffer.GetUint32();
			uint32_t sum = partMessageSize;
			int iPckt2 = 0;
            for (iPckt2 = iPckt + 1; iPckt2 < (int)httpManager._packetsIn.size(); ++iPckt2) { // Просмотрим следующие пакеты, чтобы выяснить есть ли в них все фрагменты для сбора сообщения iPckt
                auto& packetPtr2 = httpManager._packetsIn[iPckt2];
				DeserializationBuffer buffer2(packetPtr2->_packetData); // !!! Неоптимально. Сделать возможность в DeserializationBuffer хранить указатель на вектор, а не копировать в него вектор целиком
				uint8_t pType2 = buffer2.GetUint8();
				if (pType2 != PacketDataType::PartOfMessage) {
					Log("pType2 != PacketDataType::PartOfMessage"); // !!! Сделать корректный выход, если возможно
				}
				uint32_t messageSize2 = buffer2.GetUint32();
				if (messageSize != messageSize2) {
					ExitMsg("messageSize != messageSize2");
				}
				uint32_t partMessageSize2 = buffer2.GetUint32();
				sum += partMessageSize2;
				if (sum > messageSize) {
					ExitMsg("sum > messageSize");
				}
				if (sum == messageSize) {
					break; // Нашли последний пакет, содержащий данное сообщение. Выходим досрочно.
				}
			}
            if (iPckt2 < (int)httpManager._packetsIn.size()) { // Вышли досрочно, значит найдены все пакеты с данным сообщением. Склеем его и удалим содержащие его пакеты
				int lastMsgPckt = iPckt2; // Последний пакет, содержащий данные текущего пакета
                msgsQueueIn.emplace_back(std::make_unique<DeserializationBuffer>(buffer.GetNextBytes(partMessageSize), partMessageSize)); // Начнём склейку взяв данные сообщения из первого пакета
                DeserializationBuffer* pBuf = msgsQueueIn.back().get();
				for (iPckt2 = iPckt + 1; iPckt2 <= lastMsgPckt; ++iPckt2) { // Добавим данные из остальных пакетов (сразу будем удалять их)
                    auto& packetPtr2 = httpManager._packetsIn[iPckt2];
					DeserializationBuffer buffer2(packetPtr2->_packetData); // !!! Неоптимально. Сделать возможность в DeserializationBuffer хранить указатель на вектор, а не копировать в него вектор целиком
					uint8_t pType2 = buffer2.GetUint8();
					uint32_t messageSize2 = buffer2.GetUint32();
					uint32_t partMessageSize2 = buffer2.GetUint32();
					pBuf->AddBytes(buffer2.GetNextBytes(partMessageSize2), partMessageSize2);
                    httpManager._packetsIn.erase(httpManager._packetsIn.begin() + iPckt2);
					--iPckt2;
					--lastMsgPckt;
				}
                httpManager._packetsIn.erase(httpManager._packetsIn.begin() + iPckt);
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


