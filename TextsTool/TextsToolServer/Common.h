#pragma once
#include <queue>

#include "TextsBaseClasses.h"
#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "SClientMessagesMgr.h"
#include "SHttpManager.h"

class TextsDatabase;
class Folder;

//===============================================================================
//
//===============================================================================

enum class PacketDataType
{
	WholeMessages = 0,  // В пакете одно или несколько целых сообщений
	PartOfMessage = 1,  // В пакете одно незавершённое сообщение
};
inline bool operator==(const uint8_t& v1, const PacketDataType& v2) { return v1 == (uint8_t)v2; }
inline bool operator!=(const uint8_t& v1, const PacketDataType& v2) { return v1 != (uint8_t)v2; }

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
};

//===============================================================================
//
//===============================================================================

class PeriodUpdater
{
public:
	void SetPeriod(double period);
	bool IsTimeToProceed(double dt);

private:
	double _timeToProcess = 0;
	double _period = 1;
};

//===============================================================================
// Обрабатывает входящую очередь сообщений от клиентов, модифицирует базу под действием этих сообщений, 
// записывает на диск историю, добавляет сообщения в очередь на отсылку (для синхронизации всем клиентам -
// в том числе и тем, от кого пришли сообщения)
// Также обрабатывает очередь подключений и отключений. На основании этого создаёт/удаляет объекты 
// SConnectedClient. Для новых объектов накидывает в очередь сообщения о стартовой синхронизации.
//===============================================================================

class STextsToolApp;



//===============================================================================
// Сообщения из очереди исходящих сообщений переносит в очередь пакетов, оптимально объединяя или разбивая
// Также входящие пакеты перекладывает в очередь сообщений объединяя или разбивая
//===============================================================================

class SMessagesRepaker
{
public:
	SMessagesRepaker(STextsToolApp* app);
	void Update(double dt);

	STextsToolApp* _app = nullptr;
};


//===============================================================================
//
//===============================================================================

class SConnectedClient
{
public:
	using Ptr = std::shared_ptr<SConnectedClient>;

	SConnectedClient(const std::string& login, uint32_t sessionId);

	std::string _login;
	uint32_t _sessionId;
	std::string _dbName;  // Имя база, с которой сейчас работает клиент
	bool _syncFinished = false; // Ставится в true, когда в _msgsQueueOut записаны все сообщения стартовой синхронизации и значит можно добавлять сообщения синхронизации с других клиентов
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать клиенту
	std::vector<DeserializationBuffer::Ptr> _msgsQueueIn;  // Очередь пришедших от клиента сообщений
};


//===============================================================================
//
//===============================================================================

class STextsToolApp
{
public:
	STextsToolApp();
	void Update(double dt);

	std::vector<TextsDatabasePtr> _dbs;
	std::vector<SConnectedClient::Ptr> _clients;

	SClientMessagesMgr _messagesMgr;
	SHttpManager       _httpMgr;
	SMessagesRepaker   _messagesRepaker;
};

