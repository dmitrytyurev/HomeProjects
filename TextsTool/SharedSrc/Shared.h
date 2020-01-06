#pragma once
#include <stdint.h>

const int HTTP_BUF_SIZE = 500000;
const bool NEED_PACK_PACKETS = false;

//-------------------------------------------------------------------------------


namespace AttributePropertyDataType // Тип данных атрибута таблицы. Часть типов используется для сериализации/десериализации, часть типов для отображения данных в таблице
{
	enum : uint8_t
	{
		Translation_t = 0,  // Текст одного из дополнительных языков
		CommonText_t = 1,   // Текст общего назначения (не перевод)
		Checkbox_t = 2,     // Чекбокс
		BaseText_t = 3,     // Базовый текст
		Id_t = 4,           // Id-текста (строка)
		CreationTimestamp_t = 5,  // Таймстэмп создания текста
		ModificationTimestamp_t = 6, // Таймстэмп последнего текста
		LoginOfLastModifier_t = 7, // Логин того, кто последний модифицировал данный текст
	};
}

//-------------------------------------------------------------------------------

namespace FolderDataTypeForSyncMsg // Для сообщения о сихронизации базы: тип данныхоб отличающемся каталоге
{
	enum : uint8_t
	{
		AllTexts = 0,  // Пересылается информация обо всех текстах каталога (поскольку их мало)
		DiffTexts = 1, // Пересылается информация об отличающихся текстах каталога, разбитая по группам
	};
}

//-------------------------------------------------------------------------------

namespace PacketDataType
{
	enum : uint8_t
	{
		WholeMessages = 0,  // В пакете одно или несколько целых сообщений
		PartOfMessage = 1,  // В пакете одно незавершённое сообщение
	};
}

//-------------------------------------------------------------------------------

namespace ClientRequestTypes { // Типы запросов от клиента
	enum : uint8_t  
	{
		RequestConnect = 0,
		RequestPacket = 1,
		ProvidePacket = 2
	};
}

//-------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------

namespace EventType
{
	enum : uint8_t
	{
		RequestSync = 0,               // Clt->Srv: Запрос синхронизации при подключении нового клиента
		ReplySync = 1,                 // Srv->Clt: Ответ на запрос EventRequestSync
		RequestListOfDatabases = 2,    // Clt->Srv: Запрос списка баз с текстами
		ReplyListOfDatabases = 3,      // Srv->Clt: Ответ на запрос RequestListOfDatabases
		ChangeDataBase = 4,            // Srv->Clt: Сообщение об изменении базы (другим польователем или наш loopback) (см. типы событий начиная с номера 100)

		// Блок событий, приходящих с клиента на сервер, а потом рассылаемых с сервера на остальные клиенты подключенные к данной базе
		// Используются в файлах базы и истории, поэтому не могут быть изменены
		// Звёздочками помечены действия, которые на клиенте происходит только после ответа с сервера
		CreateFolder = 100,            //*Создание каталога для текстов
		DeleteFolder = 101,            //*Удаление каталога (должен не иметь текстов и вложенных каталогов)
		ChangeFolderParent = 102,      // Изменение родительского каталога
		RenameFolder = 103,            // Переименование каталога
		CreateAttribute = 104,         //*Создание атрибута таблицы
		DeleteAttribute = 105,         //*Удаление атрибута таблицы
		RenameAttribute = 106,         // Переименование атрибута таблицы
		ChangeAttributeVis = 107,      // Изменение отображаемого порядкового номера атрибута в таблице текстов или видимость атрибута
		CreateText = 108,              // Создание текста
		DeleteText = 109,              // Удаление текста
		MoveTextToFolder = 110,        // Текст переместился в другую папку
		ChangeBaseText = 111,          // Изменился основной текст
		ChangeAttributeInText = 112,   // Изменилось значение атрибута в тексте (если указанного атрибута нет в тексте, то он создаётся; если назначается пустой текст, то атрибут удаляется из текста)
	};
}

//-------------------------------------------------------------------------------
// Интервал текстов при синхронизации изменившихся каталогов
//-------------------------------------------------------------------------------

struct TextsInterval
{
	uint32_t firstTextIdx = 0; // Индекс первого текста интервала
	uint32_t textsNum = 0; // Количество текстов в интервале
	uint64_t hash = 0;  // Хэш ключей текстов интервала
};

//-------------------------------------------------------------------------------

inline void MakeKey(uint32_t tsModified, const std::string& textId, std::vector<uint8_t>& result)
{
	result.resize(sizeof(uint32_t) + textId.size());
	uint8_t* p = &result[0];

	*(reinterpret_cast<uint32_t*>(p)) = tsModified;
	memcpy(p + sizeof(uint32_t), textId.c_str(), textId.size());
}

//-------------------------------------------------------------------------------

inline bool IfKeyALess(const uint8_t* p1, int size1, const uint8_t* p2, int size2)
{
	if (*((uint32_t*)p1) < *((uint32_t*)p2)) {
		return true;
	}
	if (*((uint32_t*)p1) > *((uint32_t*)p2)) {
		return false;
	}
	p1 += sizeof(uint32_t);
	p2 += sizeof(uint32_t);
	while (true) {
		if (*p1 < *p2) {
			return true;
		}
		else {
			if (*p1 > *p2) {
				return false;
			}
		}
		--size1;
		--size2;
		if (size1 == 0 && size2 == 0) {
			return false;
		}
		if (size1 == 0) {
			return true;
		}
		if (size2 == 0) {
			return false;
		}
		++p1;
		++p2;
	}
	return false;
}
