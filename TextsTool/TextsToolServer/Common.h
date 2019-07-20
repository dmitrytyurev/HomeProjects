#pragma once
#include <string>
#include <vector>

//===============================================================================
// 
//===============================================================================

class SerializationBuffer
{
public:
	void Push(uint8_t v);
	void Push(uint32_t v);
	void Push(const std::string& v);
	void PushStringWithoutZero(const std::string& v);

	void PushBytes(const void* bytes, int size);
	uint32_t GetCurrentOffset();
	void PushBytesFromOff(uint32_t offset, const void* bytes, int size);

	std::vector<uint8_t> buffer;
};

//===============================================================================
// Запись и чтение базы текстов на диск (как целиком, так и история)
//===============================================================================

class TextsDatabase;

class DbSerializer
{
public:
	DbSerializer(TextsDatabase* pDataBase);
	void Update(float dt); // Как минимум для закрытия файла через 1 сек после записи

	void SaveDatabase();  // Имя файла базы конструирует из имени базы
	void LoadDatabaseAndHistory(); // Имена файлов базы и истории конструирует из имени базы, выбирает самые свежие файлы

//	void SaveCreateFolder(const Folder& folder);
//	void SaveRenameFolder(const Folder& folder);

	TextsDatabase* _pDataBase;
	SerializationBuffer _serializationBuffer;
};


//===============================================================================
// Свойства атрибута (колонка в базе текстов)
//===============================================================================

class AttributeProperty //: public ISerializable
{
public:

	enum DataType
	{
		Translation_t = 0,  // Текст одного из дополнительных языков
		CommonText_t = 1,   // Текст общего назначения (не перевод)
		Checkbox_t = 2,     // Чекбокс
	};

	uint8_t id;           // ID атрибута
	std::string name;     // Имя атрибута
	uint8_t type;         // Значение одного из типов DataType
	uint32_t param1;      // Параметр, зависящий от типа (для Translation_t - тут id языка)
	uint32_t param2;      // Параметр, зависящий от типа (запас)
	int visiblePosition;  // Визуальная позиция атрибута в таблице (даже если скрыт)
	bool isVisible;       // Видимость атрибута
	uint32_t timestampCreated;  // Время создания
};

//===============================================================================
// Данные атрибута в тексте
//===============================================================================

class AttributeInText //: public ISerializable
{
public:

	std::string text;   // Текстовые данные атрибута, если это текст
	uint8_t flagState;  // Состояние флажка, если это флажок;
	uint8_t id;         // ID атрибута, по которому можно узнать его тип
};


//===============================================================================
// Текст со всеми переводами и прочими свойствами (строчка в таблице)
//===============================================================================

class TextTranslated //: public ISerializable
{
public:
	using Ptr = std::shared_ptr<TextTranslated>;

	std::string id;                   // ID текста
	uint32_t timestampCreated;        // Время создания
	uint32_t timestampLastModified;   // Время последнего изменения
	std::string loginOfLastModifier;  // Логин того, кто менял последний раз
	uint32_t offsLastModified;        // смещение в файле истории до записи о последнем изменении
	std::string baseText;             // Текст на базовом языке (русском)
	std::vector<AttributeInText> attributes; // Атрибуты текста с их данными
};


//===============================================================================
// Каталог с текстами и вложенными каталогами в базе текстов
//===============================================================================

class Folder // : public ISerializable
{
public:

	uint32_t id;                 // ID папки
	uint32_t timestampCreated;   // Время создания
	uint32_t timestampModified;  // Время изменения данных папки или её текстов
	std::string name;            // Имя папки
	uint32_t parentId;                 // ID родительской папки
	std::vector<TextTranslated::Ptr> texts;  // Тексты лежащие непосредственно в папке
};

//===============================================================================
// База текстов
//===============================================================================

class TextsDatabase
{
public:

	TextsDatabase(const std::string dbName); // Загружает в объект базу из свежих файлов
	void Update(float dt);

	std::string _dbName;           // Имя базы данных текстов
	std::vector<AttributeProperty> _attributeProps; // Свойства атрибутов (колонок) таблицы
	std::vector<Folder> _folders;  // Папки. Рекурсивная структура через ссылку на ID родителя

	DbSerializer _dbSerializer; // Чтение/запись базы текстов на диск
};
