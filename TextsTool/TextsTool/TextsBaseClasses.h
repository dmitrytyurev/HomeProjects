#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <QString>
#include <qtreewidget.h>

class DeserializationBuffer;
class SerializationBuffer;
class DbSerializer;
class TextsDatabase;
class QTreeWidgetItem;

using DbSerializerPtr = std::unique_ptr<DbSerializer>;
using SerializationBufferPtr = std::shared_ptr<SerializationBuffer>;
using TextsDatabasePtr = std::shared_ptr<TextsDatabase>;


//===============================================================================
// Свойства атрибута (колонка в базе текстов)
//===============================================================================

class AttributeProperty
{
public:
	enum TS_FILTER_MODE {  // Режимы фильтра по Timestamp (время создания текста или время модификации текста)
		NO_FILTER,         // Фильтр не испольуется
		GREATER_THAN_TS,   // Фильтр проходят тексты с временем больше заданного
		LOWER_THAN_TS,     // Фильтр проходят тексты с временем меньше заданного
	};

	AttributeProperty() {}
	void LoadFullDump(DeserializationBuffer& buffer);
	void SaveFullDump(SerializationBuffer& buffer) const;

	void CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId);   // Создание объекта из сообщения от клиента 
	void Log(const std::string& prefix);
	bool IsFilteredByThisAttribute();
	bool IsSortedByThisAttribute();

	uint8_t id = 0;           // ID атрибута
	std::string name;         // Имя атрибута
	uint8_t type = 0;         // Значение одного из типов AttributePropertyType
	uint32_t param1 = 0;      // Параметр, зависящий от типа (для Translation_t - тут id языка)
	uint32_t param2 = 0;      // Параметр, зависящий от типа (запас)
	uint8_t visiblePosition = 0;    // Визуальная позиция атрибута в таблице (даже если скрыт)
	bool isVisible = false;         // Видимость атрибута
	uint32_t timestampCreated = 0;  // Время создания
	// Параметры фильтрации
	QString filterUtf8;       // (Для игровых текстов) Строка по которой фильтруется данное поле. Если пустая строка, то фильтра нет.
	std::string filterOem;    // (Для полей Id, Login) Строка по которой фильтруется данное поле. Если пустая строка, то фильтра нет.
	TS_FILTER_MODE filterCreateTsMode = NO_FILTER;  // Режим фильтра по времени создания текста
	uint32_t filterCreateTs;                        // Относительно какого времени фильтруем
	TS_FILTER_MODE filterModifyTsMode = NO_FILTER;  // Режим фильтра по времени модификации текста
	uint32_t filterModifyTs;				        // Относительно какого времени фильтруем
};

//===============================================================================
// Данные атрибута в тексте
//===============================================================================

class AttributeInText
{
public:
	AttributeInText() {}
	void LoadFullDump(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void SaveFullDump(SerializationBuffer& buffer) const;
	void Log(const std::string& prefix);

	std::string text;       // Текстовые данные атрибута, если это текст
	uint8_t uintValue = 0;  // Значение целого типа
	uint8_t id = 0;         // ID атрибута, по которому можно узнать его тип
	uint8_t type = 0;       // Значение одного из типов AttributeProperty::DataType. !!!Здесь копия для быстрого доступа! В файле не нужно, поскольку тип можно определить по ID атрибута
};


//===============================================================================
// Текст со всеми переводами и прочими свойствами (строчка в таблице)
//===============================================================================

class TextTranslated
{
public:
	TextTranslated() {}
	void LoadFullDump(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);
	void SaveFullDump(SerializationBuffer& buffer) const;
	void Log(const std::string& prefix);

	std::string id;                   // ID текста
	uint32_t timestampCreated = 0;    // Время создания
	uint32_t timestampModified = 0;   // Время последнего изменения. UINT32_MAX - признак, что были изменения из GUI. Если на сервере это изменение не применится, надо чтобы при стартовой синхронизации, была разница timestamp и данные обновились с сервера
	std::string loginOfLastModifier;  // Логин того, кто менял последний раз
	std::string baseText;             // Текст на базовом языке (русском)
	std::vector<AttributeInText> attributes; // Атрибуты текста с их данными
};

using TextTranslatedPtr = std::shared_ptr<TextTranslated>;

//===============================================================================
// Каталог с текстами и вложенными каталогами в базе текстов
//===============================================================================

class Folder
{
public:
	Folder() {}
	void LoadFullDump(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType);  // Загрузка объекта с полного дампа
	void SaveFullDump(SerializationBuffer& buffer) const;      // Запись полного дампа объекта

	void CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId);   // Создание объекта из сообщения от клиента 
	void Log(const std::string& prefix);

	uint32_t id = 0;                 // ID папки
	uint32_t timestampCreated = 0;   // Время создания
	uint32_t timestampModified = 0;  // Время изменения данных папки или её текстов. UINT32_MAX - признак, что были изменения из GUI. Если на сервере это изменение не применится, надо чтобы при стартовой синхронизации, была разница timestamp и данные обновились с сервера
	std::string name;                // Имя папки
	uint32_t parentId = 0;           // ID родительской папки, для корневой папки здесь UINT32_MAX
	std::vector<TextTranslatedPtr> texts;  // Тексты лежащие непосредственно в папке
	std::unique_ptr<QTreeWidgetItem> uiTreeItem; // Указатель на айтем в UI
};

//===============================================================================
// База текстов
//===============================================================================

class TextsDatabase
{
public:
	TextsDatabase(const std::string path, const std::string dbName);
	void LogDatabase();

	bool isSynced = false;         // Синхронизирована ли база с сервером
	std::string _dbName;           // Имя базы данных текстов
	std::vector<AttributeProperty> _attributeProps; // Свойства атрибутов (колонок) таблицы
	std::vector<Folder> _folders;  // Папки. Рекурсивная структура через ссылку на ID родителя

	DbSerializerPtr _dbSerializer; // Чтение/запись базы текстов на диск
};

