#pragma once

const int HTTP_BUF_SIZE = 500000;
const bool NEED_PACK_PACKETS = false;

//===============================================================================
//
//===============================================================================

enum PacketDataType
{
	WholeMessages = 0,  // В пакете одно или несколько целых сообщений
	PartOfMessage = 1,  // В пакете одно незавершённое сообщение
};

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
	EventRequestSync = 0,               // Запрос синхронизации при подключении нового клиента
	EventReplySync = 1,                 // Ответ на запрос EventRequestSync
	EventRequestListOfDatabases = 2,    // Запрос списка баз с текстами
	EventReplyListOfDatabases = 3,      // Ответ на запрос EventRequestListOfDatabases

	// Блок событий, приходящих с клиента на сервер, а потом рассылаемых с сервера на остальные клиенты подключенные к данной базе
	// Используются в файлах базы и истории, поэтому не могут быть изменены
	EventCreateFolder = 100,            // Создание каталога для текстов
	EventDeleteFolder = 101,            // Удаление каталога (должен не иметь текстов и вложенных каталогов)
	EventChangeFolderParent = 102,      // Изменение родительского каталога
	EventRenameFolder = 103,            // Переименование каталога
	EventCreateAttribute = 104,         // Создание атрибута таблицы
	EventDeleteAttribute = 105,         // Удаление атрибута таблицы
	EventRenameAttribute = 106,         // Переименование атрибута таблицы
	EventChangeAttributeVis = 107,      // Изменение отображаемого порядкового номера атрибута в таблице текстов или видимость атрибута
	EventCreateText = 108,              // Создание текста
	EventDeleteText = 109,              // Удаление текста
	EventMoveTextToFolder = 110,        // Текст переместился в другую папку
	EventChangeBaseText = 111,          // Изменился основной текст
	EventAddAttributeToText = 112,      // В текст добавился атрибут
	EventDelAttributeFromText = 113,    // Удалился атрибут из текста
	EventChangeAttributeInText = 114,   // Изменилось значение атрибута в тексте
};
