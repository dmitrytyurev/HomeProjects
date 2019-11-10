#pragma once

const int HTTP_BUF_SIZE = 500000;
const bool NEED_PACK_PACKETS = false;

enum class ClientRequestTypes // Типы запросов от клиента
{
    RequestConnect = 0,
    RequestPacket = 1,
    ProvidePacket = 2
};

enum class AnswersToClient // Коды ответов
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

enum class RepackerDataType // Тип данных добавленных репакером перед пакетом 
{
	ONE_OR_MORE_ENTIRE_MESSAGES = 0,  // В этом пакете одно или несколько целых сообщений
	PART_OF_ONE_MESSAGE = 1  // В этом пакете часть одного сообщения
};