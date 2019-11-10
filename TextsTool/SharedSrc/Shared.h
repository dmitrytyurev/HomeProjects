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

//===============================================================================
//
//===============================================================================

enum EventType
{
	EventRequestSync = 0,          // Запрос синхронизации при подключении нового клиента
	EventCreateFolder = 1,         // Создание каталога для текстов
	EventDeleteFolder = 2,         // Удаление каталога (должен не иметь текстов и вложенных каталогов)
	EventChangeFolderParent = 3,   // Изменение родительского каталога
	EventRenameFolder = 4,         // Переименование каталога
	EventCreateAttribute = 5,      // Создание атрибута таблицы
	EventDeleteAttribute = 6,      // Удаление атрибута таблицы
	EventRenameAttribute = 7,      // Переименование атрибута таблицы
	EventChangeAttributeVis = 8,   // Изменение отображаемого порядкового номера атрибута в таблице текстов или видимость атрибута
	EventCreateText = 9,           // Создание текста
	EventDeleteText = 10,           // Удаление текста
	EventMoveTextToFolder = 11,    // Текст переместился в другую папку
	EventChangeBaseText = 12,      // Изменился основной текст
	EventAddAttributeToText = 13,  // В текст добавился атрибут
	EventDelAttributeFromText = 14,  // Удалился атрибут из текста
	EventChangeAttributeInText = 15, // Изменилось значение атрибута в тексте
	EventRequestListOfDatabases = 16,       // Запрос список баз с текстами
};
