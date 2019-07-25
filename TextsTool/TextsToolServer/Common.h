#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

//===============================================================================
// 
//===============================================================================

class SerializationBuffer
{
public:
	void Push(uint8_t v);
	void Push(uint16_t v);
	void Push(uint32_t v);
	void Push(const std::string& v);
	void PushStringWithoutZero(const std::string& v);

	template <typename T>
	void PushStringWithoutZero(const std::string& v);


	void PushBytes(const void* bytes, int size);

	std::vector<uint8_t> buffer;
};

//===============================================================================
// 
//===============================================================================

class DeserializationBuffer
{
public:
	template <typename T>
	T GetUint();
	template <typename T>
	void GetString(std::string& result);
	bool IsEmpty() { return buffer.size()-1 == offset; }

	uint32_t offset = 0;
	std::vector<uint8_t> buffer;
};

//===============================================================================
// Запись и чтение базы текстов на диск (как целиком, так и история)
//===============================================================================

class TextsDatabase;
class Folder;

class DbSerializer
{
public:
	enum ActionType
	{
		ActionCreateFolder = 0,         // Создание каталога для текстов
	};

	struct HistoryFile
	{
		std::string name;                // Имя файла истории текущей базы (пустая строка, если файл истории ещё не создавался - с момента чтения/записи основного файла не было событий)
		double timeToFlush = 0;          // Время в секундах до флаша буфера истории на диск
		SerializationBuffer buffer;
	};

	DbSerializer(TextsDatabase* pDataBase);
	void SetPath(const std::string& path);
	void Update(double dt);

	void SaveDatabase();  // Имя файла базы конструирует из имени базы
	void LoadDatabaseAndHistory(); // Имена файлов базы и истории конструирует из имени базы, выбирает самые свежие файлы

	void HistoryFlush();
	SerializationBuffer& GetSerialBuffer();
	static void PushCommonHeader(SerializationBuffer& buffer, uint32_t timestamp, const std::string& loginOfLastModifier, uint8_t actionType);

private:
	std::string FindFreshBaseFileName(uint32_t& resultTimestamp);
	std::string FindHistoryFileName(uint32_t tsBaseFile);
	void LoadDatabaseInner(const std::string& fullFileName);
	void LoadHistoryInner(const std::string& fullFileName);

	std::string _path;            // Путь, где хранятся файлы базы и файлы истории
	HistoryFile _historyFile;     // Информация о файле истории
	TextsDatabase* _pDataBase = nullptr;
};

//===============================================================================
// Свойства атрибута (колонка в базе текстов)
//===============================================================================

class AttributeProperty
{
public:
	enum DataType
	{
		Translation_t = 0,  // Текст одного из дополнительных языков
		CommonText_t = 1,   // Текст общего назначения (не перевод)
		Checkbox_t = 2,     // Чекбокс
	};

	AttributeProperty() {}
	AttributeProperty(DeserializationBuffer& buffer);
	void SaveToBase(SerializationBuffer& buffer) const;

	uint8_t id = 0;           // ID атрибута
	std::string name;         // Имя атрибута
	uint8_t type = 0;         // Значение одного из типов DataType
	uint32_t param1 = 0;      // Параметр, зависящий от типа (для Translation_t - тут id языка)
	uint32_t param2 = 0;      // Параметр, зависящий от типа (запас)
	uint8_t visiblePosition = 0;    // Визуальная позиция атрибута в таблице (даже если скрыт)
	bool isVisible = false;         // Видимость атрибута
	uint32_t timestampCreated = 0;  // Время создания
};

//===============================================================================
// Данные атрибута в тексте
//===============================================================================

class AttributeInText
{
public:
	AttributeInText() {}
	AttributeInText(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void SaveToBase(SerializationBuffer& buffer) const;

	std::string text;       // Текстовые данные атрибута, если это текст
	uint8_t flagState = 0;  // Состояние флажка, если это флажок;
	uint8_t id = 0;         // ID атрибута, по которому можно узнать его тип
	uint8_t type = 0;       // Значение одного из типов AttributeProperty::DataType. !!!Здесь копия для быстрого доступа! В файле не нужно, поскольку тип можно определить по ID атрибута
};


//===============================================================================
// Текст со всеми переводами и прочими свойствами (строчка в таблице)
//===============================================================================

class TextTranslated
{
public:
	using Ptr = std::shared_ptr<TextTranslated>;

	TextTranslated() {}
	TextTranslated(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void SaveToBase(SerializationBuffer& buffer) const;

	std::string id;                   // ID текста
	uint32_t timestampCreated = 0;    // Время создания
	uint32_t timestampModified = 0;   // Время последнего изменения
	std::string loginOfLastModifier;  // Логин того, кто менял последний раз
	uint32_t offsLastModified = 0;    // смещение в файле истории до записи о последнем изменении
	std::string baseText;             // Текст на базовом языке (русском)
	std::vector<AttributeInText> attributes; // Атрибуты текста с их данными
};


//===============================================================================
// Каталог с текстами и вложенными каталогами в базе текстов
//===============================================================================

class Folder
{
public:
	Folder() {}
	void CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);  // Создание объекта из полного файла базы
	void CreateFromHistory(DeserializationBuffer& buffer);  // Создание объекта из файла истории
	void CreateFromPacket(DeserializationBuffer& buffer);   // Создание объекта из сообщения от клиента 
	void SaveToBase(SerializationBuffer& buffer) const;      // Запись объекта в полный файл базы
	void SaveToHistory(SerializationBuffer& buffer, const std::string& loginOfLastModifier);

	uint32_t id = 0;                 // ID папки
	uint32_t timestampCreated = 0;   // Время создания
	uint32_t timestampModified = 0;  // Время изменения данных папки или её текстов
	std::string name;                // Имя папки
	uint32_t parentId = 0;           // ID родительской папки
	std::vector<TextTranslated::Ptr> texts;  // Тексты лежащие непосредственно в папке
};

//===============================================================================
// База текстов
//===============================================================================

class TextsDatabase
{
public:
	using Ptr = std::shared_ptr<TextsDatabase>;

	TextsDatabase(const std::string path, const std::string dbName); // Загружает в объект базу из свежих файлов
	void Update(double dt);
	SerializationBuffer& GetSerialBuffer();

	std::string _dbName;           // Имя базы данных текстов
	uint8_t _newAttributeId = 0;   // Когда пользователь создаёт новый атрибут, берём этот номер. Поле инкрементим.
	std::vector<AttributeProperty> _attributeProps; // Свойства атрибутов (колонок) таблицы
	uint32_t _newFolderId = 0;     // Когда пользователь создаёт новую папку, берём этот номер. Поле инкрементим.
	std::vector<Folder> _folders;  // Папки. Рекурсивная структура через ссылку на ID родителя

	DbSerializer _dbSerializer; // Чтение/запись базы текстов на диск
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

class SClientMessagesMgr
{
public:
	SClientMessagesMgr(STextsToolApp* app);
	void Update(double dt);

	STextsToolApp* _app;

private:
	TextsDatabase::Ptr GetDbPtrByDbName(const std::string& dbName);
};

//===============================================================================
//
//===============================================================================

class MsgInQueue
{
public:
	using Ptr = std::unique_ptr<MsgInQueue>;

	uint8_t actionType; // Одно из значений DbSerializer::ActionType
	DeserializationBuffer _msgData; // Сериализованные данные события полученные от клиента и готовые к отсылке на клиент
};

//===============================================================================
//
//===============================================================================

class MutexLock
{
public:
	MutexLock(std::mutex& mutex) { pMutex = &mutex; mutex.lock(); }
	~MutexLock() { pMutex->unlock();  }
private:
	std::mutex* pMutex;
};

//===============================================================================
//
//===============================================================================

class HttpPacket
{
public:
	using Ptr = std::unique_ptr<HttpPacket>;

	uint32_t packetIndex; // Порядковый номер пакета
	std::vector<uint8_t> packetData;
};

//===============================================================================
//
//===============================================================================

class MTQueue
{
public:
	std::vector<HttpPacket::Ptr> queue;
	std::mutex mutex;
};

//===============================================================================
//
//===============================================================================

class SConnectedClientLow
{
	std::string login;
	std::string password;
	uint32_t timestampLastRequest;  // Когда от этого клиента приходил последний запрос
	MTQueue packetsQueueOut;
	MTQueue packetsQueueIn;
	uint32_t lastRecievedPacketN;
};

//===============================================================================
//
//===============================================================================

class SConnectedClient
{
public:
	using Ptr = std::shared_ptr<SConnectedClient>;

	std::string login;
	std::string _dbName;  // Имя база, с которой сейчас работает клиент
	std::vector<MsgInQueue::Ptr> _msgsQueueOut; // Очередь сообщений, которые нужно отослать клиенту
	std::vector<MsgInQueue::Ptr> _msgsQueueIn;  // Очередь пришедших от клиента сообщений
};

//===============================================================================
//
//===============================================================================

class STextsToolApp
{
public:
	STextsToolApp();
	void Update(double dt);

	std::vector<TextsDatabase::Ptr> _dbs;
	std::vector<SConnectedClient::Ptr> _clients;

	SClientMessagesMgr _messagesMgr;

	//	SHttpManager httpManager;
	//	SMessagesRepaker messagesRepaker;
};




