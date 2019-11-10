#pragma once

const int HTTP_BUF_SIZE = 500000;
const bool NEED_PACK_PACKETS = false;

enum class ClientRequestTypes // ���� �������� �� �������
{
    RequestConnect = 0,
    RequestPacket = 1,
    ProvidePacket = 2
};

enum class AnswersToClient // ���� �������
{
    UnknownRequest = 0,
    WrongLoginOrPassword = 1,
    WrongSession = 2,
    ClientNotConnected = 3,
    NoSuchPacketYet = 4,
    Connected = 5,
    PacketReceived = 6,
    PacketSent = 7
};

enum class RepackerDataType // ��� ������ ����������� ��������� ����� ������� 
{
	ONE_OR_MORE_ENTIRE_MESSAGES = 0,  // � ���� ������ ���� ��� ��������� ����� ���������
	PART_OF_ONE_MESSAGE = 1  // � ���� ������ ����� ������ ���������
};