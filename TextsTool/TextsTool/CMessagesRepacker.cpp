#include <memory>

#include "CMessagesRepacker.h"
#include "../SharedSrc/Shared.h"

void Log(const std::string& str);

//===============================================================================

void Repacker::RepackMessagesOutToPackets(std::vector<SerializationBufferPtr>& msgsQueueOut, CHttpManager& httpManager)
{
	const int packetMaxSize = HTTP_BUF_SIZE - 200; // ������������ ������ ������ ��������� � ������. ������ ������ ������� ������ ��������� HTTP-�������, ������, ��� � ������ ��������� ��� ����������� ������ �������� � ������ HTTP-���������

    auto& v = msgsQueueOut;

	while (v.size()) {
		if (v[0]->buffer.size() > packetMaxSize) {
			// ��������� ��������� ������������ ������ ������. ��������� ��� �� ��������� �������
			int n = (v[0]->buffer.size() + packetMaxSize - 1) / packetMaxSize; // ����� �������, �� ������� �������� ���������
			int offs = 0;
			for (int i = 0; i < n; ++i) {
                int curSize =(i == n - 1 ? (int)v[0]->buffer.size() - (n - 1) * packetMaxSize : packetMaxSize);
				SerializationBuffer sbuf;
				sbuf.PushUint8(PacketDataType::PartOfMessage);
				sbuf.PushUint32(v[0]->buffer.size()); // ����� ������� ���������
				sbuf.PushUint32(curSize);  // ����� �������� ����� ���������
				sbuf.PushBytes(v[0]->buffer.data() + offs, curSize);
				offs += curSize;
                httpManager.PutPacketToSendQueue(sbuf.buffer);
			}
			v.erase(v.begin());
		}
		else {
			// ���������� �� ��������� ��������� � ����� (�� 1 �� N ���������)
			int n = 0;
			int sum = 0;
			// ������� ����� ��������� n, ������� ���������� � ���� �����
            while (sum + v[n]->buffer.size() + sizeof(uint32_t) <= packetMaxSize) {  // ����������� sizeof(uint32_t) ������, ��� � ������� ���������
                sum += (int)v[n]->buffer.size() + (int)sizeof(uint32_t);                       // � ������ ��������� ��� ����� - ��� ���������� ������� ����� ������
                if (++n == (int)v.size()) {
					break;
				}
			};
			if (n == 0) {
				n = 1; // ���� ����� ������ ������ � ��������� [packetMaxSize-sizeof(uint32_t)..packetMaxSize], �� ������� n == 0
			}
			// ������� n ��������� ��������� � ���� �����
			SerializationBuffer sbuf;
			sbuf.PushUint8(PacketDataType::WholeMessages);
			sbuf.PushUint32(n); // ���������� ����� ��������� � ������
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
		DeserializationBuffer buffer(packetPtr->_packetData); // !!! ������������. ������� ����������� � DeserializationBuffer ������� ��������� �� ������, � �� ���������� � ���� ������ �������
		uint8_t pType = buffer.GetUint8();
		if (pType == PacketDataType::WholeMessages) { // � ������ ������ ���� ��� ��������� ����� ���������
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
		else if (pType == PacketDataType::PartOfMessage) { // � ������ ������ ������ �������� ��������� ���������
			uint32_t messageSize = buffer.GetUint32();
			uint32_t partMessageSize = buffer.GetUint32();
			uint32_t sum = partMessageSize;
			int iPckt2 = 0;
            for (iPckt2 = iPckt + 1; iPckt2 < (int)httpManager._packetsIn.size(); ++iPckt2) { // ���������� ��������� ������, ����� �������� ���� �� � ��� ��� ��������� ��� ����� ��������� iPckt
                auto& packetPtr2 = httpManager._packetsIn[iPckt2];
				DeserializationBuffer buffer2(packetPtr2->_packetData); // !!! ������������. ������� ����������� � DeserializationBuffer ������� ��������� �� ������, � �� ���������� � ���� ������ �������
				uint8_t pType2 = buffer2.GetUint8();
				if (pType2 != PacketDataType::PartOfMessage) {
					Log("pType2 != PacketDataType::PartOfMessage"); // !!! ������� ���������� �����, ���� ��������
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
					break; // ����� ��������� �����, ���������� ������ ���������. ������� ��������.
				}
			}
            if (iPckt2 < (int)httpManager._packetsIn.size()) { // ����� ��������, ������ ������� ��� ������ � ������ ����������. ������ ��� � ������ ���������� ��� ������
				int lastMsgPckt = iPckt2; // ��������� �����, ���������� ������ �������� ������
                msgsQueueIn.emplace_back(std::make_unique<DeserializationBuffer>(buffer.GetNextBytes(partMessageSize), partMessageSize)); // ����� ������� ���� ������ ��������� �� ������� ������
                DeserializationBuffer* pBuf = msgsQueueIn.back().get();
				for (iPckt2 = iPckt + 1; iPckt2 <= lastMsgPckt; ++iPckt2) { // ������� ������ �� ��������� ������� (����� ����� ������� ��)
                    auto& packetPtr2 = httpManager._packetsIn[iPckt2];
					DeserializationBuffer buffer2(packetPtr2->_packetData); // !!! ������������. ������� ����������� � DeserializationBuffer ������� ��������� �� ������, � �� ���������� � ���� ������ �������
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
			else { // ��������� ��� ���������� ������, �� �� ��������� ������� ���� �� ���������. ������� �� ������� ������� �������� �������. 
				break;
			}
		}
		else {
			Log("Unknown packet type");
		}
	}
}


