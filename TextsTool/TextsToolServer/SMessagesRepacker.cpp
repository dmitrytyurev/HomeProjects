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
		// �������, ���� ���� �� ����������� (��� ��� ������ SConnectedClient ���� SConnectedClientLow ��� ����������� �����, 
		// ��� ��� �� SConnectedClient �� ����� ���� ����� ������ ���������, ��� ��� �� ���������)
		client->_msgsQueueOut.resize(0);
	}

	const int packetMaxSize = HTTP_BUF_SIZE - 200; // ������������ ������ ������ ��������� � ������. ������ ������ ������� ������ ��������� HTTP-�������, ������, ��� � ������ ��������� ��� ����������� ������ �������� � ������ HTTP-���������

	auto& v = client->_msgsQueueOut;

	while (v.size()) {
		if (v[0]->buffer.size() > packetMaxSize) {
			// ��������� ��������� ������������ ������ ������. ��������� ��� �� ��������� �������
			int n = (v[0]->buffer.size() + packetMaxSize - 1) / packetMaxSize; // ����� �������, �� ������� �������� ���������
			int offs = 0;
			for (int i = 0; i < n; ++i) {
				int curSize = (i == n - 1 ? v[0]->buffer.size() - (n - 1) * packetMaxSize : packetMaxSize);
				SerializationBuffer sbuf;
				sbuf.Push((uint8_t)PacketDataType::PartOfMessage);
				sbuf.Push((uint32_t)v[0]->buffer.size()); // ����� ������� ���������
				sbuf.Push((uint32_t)curSize);  // ����� �������� ����� ���������
				sbuf.PushBytes(v[0]->buffer.data() + offs, curSize);
				offs += curSize;
				clLow->_packetsQueueOut.PushPacket(sbuf.buffer, HttpPacket::Status::WAITING_FOR_PACKING);
			}
			v.erase(v.begin());
		}
		else {
			// ���������� �� ��������� ��������� � ����� (�� 1 �� N ���������)
			int n = 0;
			int sum = 0;
			// ������� ����� ��������� n, ������� ���������� � ���� �����
			while (sum + v[n]->buffer.size() + sizeof(uint32_t) <= packetMaxSize) {  // ����������� sizeof(uint32_t) ������, ��� � ������� ���������
				sum += v[n]->buffer.size() + sizeof(uint32_t);                       //                                                              � ������ ��������� ��� ����� - ��� ���������� ������� ����� ������
				if (++n == v.size()) {
					break;
				}
			};
			if (n == 0) {
				n = 1; // ���� ����� ������ ������ � ��������� [packetMaxSize-sizeof(uint32_t)..packetMaxSize], �� ������� n == 0
			}
			// ������� n ��������� ��������� � ���� �����
			SerializationBuffer sbuf;
			sbuf.Push(PacketDataType::WholeMessages);
			sbuf.Push((uint32_t)n); // ���������� ����� ��������� � ������
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
		// ���� ��� �� ��������, ������ �������� ����� SConnectedClientLow, � SConnectedClient ��� �� ����� �������������, 
		// �� ������� ��� �� ��������� Update � ����� �� ����� ����� ���� � ��������� ������ ��� � ������ SConnectedClient
		return;
	}

	for (int iPckt = 0; iPckt < (int)clLow->_packetsQueueIn.size(); ++iPckt) {
		auto& packetPtr = clLow->_packetsQueueIn[iPckt];
		DeserializationBuffer buffer(packetPtr->_packetData); // !!! ������������. ������� ����������� � DeserializationBuffer ������� ��������� �� ������, � �� ���������� � ���� ������ �������
		Utils::LogBuf(buffer._buffer);
		uint8_t pType = buffer.GetUint<uint8_t>();
		if (pType == PacketDataType::WholeMessages) { // � ������ ������ ���� ��� ��������� ����� ���������
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
		else if (pType == PacketDataType::PartOfMessage) { // � ������ ������ ������ �������� ��������� ���������
			uint32_t messageSize = buffer.GetUint<uint32_t>();
			uint32_t partMessageSize = buffer.GetUint<uint32_t>();
			uint32_t sum = partMessageSize;
			int iPckt2 = 0;
			for (iPckt2 = iPckt + 1; iPckt2 < (int)clLow->_packetsQueueIn.size(); ++iPckt2) { // ���������� ��������� ������, ����� �������� ���� �� � ��� ��� ��������� ��� ����� ��������� iPckt
				auto& packetPtr2 = clLow->_packetsQueueIn[iPckt2];
				DeserializationBuffer buffer2(packetPtr2->_packetData); // !!! ������������. ������� ����������� � DeserializationBuffer ������� ��������� �� ������, � �� ���������� � ���� ������ �������
				uint8_t pType2 = buffer2.GetUint<uint8_t>();
				if (pType2 != PacketDataType::PartOfMessage) {
					Log("pType2 != PacketDataType::PartOfMessage"); // !!! ������� ���������� �����, ���� ��������
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
					break; // ����� ��������� �����, ���������� ������ ���������. ������� ��������.
				}
			}
			if (iPckt2 < (int)clLow->_packetsQueueIn.size()) { // ����� ��������, ������ ������� ��� ������ � ������ ����������. ������ ��� � ������ ���������� ��� ������
				int lastMsgPckt = iPckt2; // ��������� �����, ���������� ������ �������� ������
				client->_msgsQueueIn.emplace_back(std::make_unique<DeserializationBuffer>(buffer.GetNextBytes(partMessageSize), partMessageSize)); // ����� ������� ���� ������ ��������� �� ������� ������
				DeserializationBuffer* pBuf = client->_msgsQueueIn.back().get();
				for (iPckt2 = iPckt + 1; iPckt2 <= lastMsgPckt; ++iPckt2) { // ������� ������ �� ��������� ������� (����� ����� ������� ��)
					auto& packetPtr2 = clLow->_packetsQueueIn[iPckt2];
					DeserializationBuffer buffer2(packetPtr2->_packetData); // !!! ������������. ������� ����������� � DeserializationBuffer ������� ��������� �� ������, � �� ���������� � ���� ������ �������
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
			else { // ��������� ��� ���������� ������, �� �� ��������� ������� ���� �� ���������. ������� �� ������� ������� �������� �������. 
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

