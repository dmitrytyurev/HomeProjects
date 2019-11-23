#pragma once
#include <stdint.h>

const int HTTP_BUF_SIZE = 500000;
const bool NEED_PACK_PACKETS = false;

//===============================================================================
//
//===============================================================================

namespace PacketDataType
{
	enum : uint8_t
	{
		WholeMessages = 0,  // В пакете одно или несколько целых сообщений
		PartOfMessage = 1,  // В пакете одно незавершённое сообщение
	};
}


namespace ClientRequestTypes { // Типы запросов от клиента
	enum : uint8_t  
	{
		RequestConnect = 0,
		RequestPacket = 1,
		ProvidePacket = 2
	};
}

namespace AnswersToClient // Коды ответов
{
	enum : uint8_t 
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
}

//===============================================================================
//
//===============================================================================

namespace EventType
{
	enum : uint8_t
	{
		RequestSync = 0,               // Запрос синхронизации при подключении нового клиента
		ReplySync = 1,                 // Ответ на запрос EventRequestSync
		RequestListOfDatabases = 2,    // Запрос списка баз с текстами
		ReplyListOfDatabases = 3,      // Ответ на запрос RequestListOfDatabases

		// Блок событий, приходящих с клиента на сервер, а потом рассылаемых с сервера на остальные клиенты подключенные к данной базе
		// Используются в файлах базы и истории, поэтому не могут быть изменены
		CreateFolder = 100,            // Создание каталога для текстов
		DeleteFolder = 101,            // Удаление каталога (должен не иметь текстов и вложенных каталогов)
		ChangeFolderParent = 102,      // Изменение родительского каталога
		RenameFolder = 103,            // Переименование каталога
		CreateAttribute = 104,         // Создание атрибута таблицы
		DeleteAttribute = 105,         // Удаление атрибута таблицы
		RenameAttribute = 106,         // Переименование атрибута таблицы
		ChangeAttributeVis = 107,      // Изменение отображаемого порядкового номера атрибута в таблице текстов или видимость атрибута
		CreateText = 108,              // Создание текста
		DeleteText = 109,              // Удаление текста
		MoveTextToFolder = 110,        // Текст переместился в другую папку
		ChangeBaseText = 111,          // Изменился основной текст
		AddAttributeToText = 112,      // В текст добавился атрибут
		DelAttributeFromText = 113,    // Удалился атрибут из текста
		ChangeAttributeInText = 114,   // Изменилось значение атрибута в тексте
	};
}
