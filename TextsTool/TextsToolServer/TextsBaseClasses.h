#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

class DeserializationBuffer;
class SerializationBuffer;
class DbSerializer;
class TextsDatabase;

using DbSerializerPtr = std::unique_ptr<DbSerializer>;
using SerializationBufferPtr = std::shared_ptr<SerializationBuffer>;
using TextsDatabasePtr = std::shared_ptr<TextsDatabase>;


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
	void CreateFromBase(DeserializationBuffer& buffer);
	void SaveToBase(SerializationBuffer& buffer) const;

	void CreateFromHistory(DeserializationBuffer& buffer, uint32_t ts);  // Создание объекта из файла истории
	void SaveToHistory(TextsDatabasePtr db, const std::string& loginOfLastModifier) const;

	void CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId);   // Создание объекта из сообщения от клиента 
	SerializationBufferPtr SaveToPacket(const std::string& loginOfModifier) const;      // Запись объекта в пакет для рассылки всем клиентам, работающим с этой базой

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
	void CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
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
	void CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void SaveToBase(SerializationBuffer& buffer) const;

	void SaveToHistory(TextsDatabasePtr db, uint32_t folderId);

	SerializationBufferPtr SaveToPacket(uint32_t folderId, const std::string& loginOfModifier) const;      // Запись объекта в пакет для рассылки всем клиентам, работающим с этой базой


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
	void SaveToBase(SerializationBuffer& buffer) const;      // Запись объекта в полный файл базы

	void CreateFromHistory(DeserializationBuffer& buffer, uint32_t ts);  // Создание объекта из файла истории
	void SaveToHistory(TextsDatabasePtr db, const std::string& loginOfLastModifier) const;

	void CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId);   // Создание объекта из сообщения от клиента 
	SerializationBufferPtr SaveToPacket(const std::string& loginOfModifier) const;      // Запись объекта в пакет для рассылки всем клиентам, работающим с этой базой

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
	void CreateDatabase(const std::string path, const std::string dbName); // Создаёт базу в памяти, создаёт пустую базу на диске
	void LoadDatabase(const std::string path, const std::string dbName); // Создаёт базу в памяти из файла базы и файла истории
	void Update(double dt);
	SerializationBuffer& GetHistoryBuffer();
	uint32_t GetCurrentPosInHistoryFile();

	std::string _dbName;           // Имя базы данных текстов
	uint8_t _newAttributeId = 0;   // Когда пользователь создаёт новый атрибут, берём этот номер. Поле инкрементим.
	std::vector<AttributeProperty> _attributeProps; // Свойства атрибутов (колонок) таблицы
	uint32_t _newFolderId = 0;     // Когда пользователь создаёт новую папку, берём этот номер. Поле инкрементим.
	std::vector<Folder> _folders;  // Папки. Рекурсивная структура через ссылку на ID родителя

	DbSerializerPtr _dbSerializer; // Чтение/запись базы текстов на диск
};

