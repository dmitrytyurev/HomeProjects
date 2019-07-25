#include "pch.h"

#include <iostream>
#include <ctime>
#include <iostream>
#include <experimental/filesystem>

#include "Common.h"

#define CHECK_BUFFER 1

void ExitMsg(const std::string& message)
{
	std::cout << "Fatal: " << message << std::endl;
	exit(1);
}

//===============================================================================
//
//===============================================================================

void PeriodUpdater::SetPeriod(double period)
{
	_period = period;
}

//===============================================================================
//
//===============================================================================

bool PeriodUpdater::IsTimeToProceed(double dt)
{
	_timeToProcess -= dt;
	bool isTimeToProceed = (_timeToProcess <= 0);

	if (isTimeToProceed) {
		_timeToProcess += _period;
	}
	return isTimeToProceed;
}

//===============================================================================
//
//===============================================================================

template <typename T>
T DeserializationBuffer::GetUint()
{
#if CHECK_BUFFER
	if (offset + sizeof(T) > buffer.size()) {
		ExitMsg("GetUint buffer overrun");
	}
#endif
	T val = *(reinterpret_cast<T*>(buffer.data() + offset));
	offset += sizeof(T);
	return val;
}

//===============================================================================
//
//===============================================================================

template <typename T>
void DeserializationBuffer::GetString(std::string& result)
{
#if CHECK_BUFFER
	if (offset + sizeof(T) > buffer.size()) {
		ExitMsg("GetString buffer overrun");
	}
#endif
	T textLength = *(reinterpret_cast<T*>(buffer.data() + offset));
	offset += sizeof(T);
	uint8_t keep = buffer[offset + textLength];
	buffer[offset + textLength] = 0;
	
#if CHECK_BUFFER
	if (offset + 1 > buffer.size()) {
		ExitMsg("GetString buffer overrun");
	}
#endif
	result = reinterpret_cast<const char*>(buffer.data() + offset);
	buffer[offset + textLength] = keep;
	offset += textLength;
}

//===============================================================================
//
//===============================================================================

void SerializationBuffer::Push(const std::string& v)
{
	const uint8_t* beg = reinterpret_cast<const uint8_t*>(v.c_str());
	const uint8_t* end = beg + v.length() + 1;

	buffer.insert(buffer.end(), beg, end);
}

//===============================================================================
//
//===============================================================================

void SerializationBuffer::PushStringWithoutZero(const std::string& v)
{
	const uint8_t* beg = reinterpret_cast<const uint8_t*>(v.c_str());
	const uint8_t* end = beg + v.length();

	buffer.insert(buffer.end(), beg, end);
}

//===============================================================================
//
//===============================================================================

template <typename T>
void SerializationBuffer::PushStringWithoutZero(const std::string& v)
{
	T nameLength = static_cast<T>(v.length());
	Push(nameLength);

	const uint8_t* beg = reinterpret_cast<const uint8_t*>(v.c_str());
	const uint8_t* end = beg + v.length();

	buffer.insert(buffer.end(), beg, end);
}

//===============================================================================
//
//===============================================================================

void SerializationBuffer::Push(uint8_t v)
{
	const uint8_t* beg = &v;
	const uint8_t* end = beg + sizeof(v);

	buffer.insert(buffer.end(), beg, end);
}

//===============================================================================
//
//===============================================================================

void SerializationBuffer::Push(uint16_t v)
{
	const uint8_t* beg = reinterpret_cast<const uint8_t*>(&v);
	const uint8_t* end = beg + sizeof(v);

	buffer.insert(buffer.end(), beg, end);
}

//===============================================================================
//
//===============================================================================

void SerializationBuffer::Push(uint32_t v)
{
	const uint8_t* beg = reinterpret_cast<const uint8_t*>(&v);
	const uint8_t* end = beg + sizeof(v);

	buffer.insert(buffer.end(), beg, end);
}

//===============================================================================
//
//===============================================================================

void SerializationBuffer::PushBytes(const void* bytes, int size)
{
	const uint8_t* beg = reinterpret_cast<const uint8_t*>(bytes);
	const uint8_t* end = beg + size;

	buffer.insert(buffer.end(), beg, end);
}


//===============================================================================
// Загружает в объект базу из свежих файлов
//===============================================================================

TextsDatabase::TextsDatabase(const std::string path, const std::string dbName): _dbSerializer(this), _dbName(dbName)
{
	_dbSerializer.SetPath(path);
	_dbSerializer.LoadDatabaseAndHistory();
}

//===============================================================================
//
//===============================================================================

void TextsDatabase::Update(double dt)
{
	_dbSerializer.Update(dt);
}

//===============================================================================
//
//===============================================================================

DbSerializer::DbSerializer(TextsDatabase* pDataBase): _pDataBase(pDataBase)
{
}

//===============================================================================
//
//===============================================================================

void DbSerializer::HistoryFlush()
{
	const double HISTORY_FLUSH_INTERVAL = 1.f;
	
	_historyFile.timeToFlush = HISTORY_FLUSH_INTERVAL;
	bool isJustCreated = false;
	std::ofstream file;

	if (_historyFile.name.empty()) {
		std::string timestamp = std::to_string(std::time(0));
		_historyFile.name = "TextsHistory_" + _pDataBase->_dbName + "_" + timestamp + ".bin";
		std::string fullFileName = _path + _historyFile.name;
		file.open(fullFileName, std::ios::out | std::ios::app | std::ios::binary);
		if (file.rdstate()) {
			ExitMsg("Error creating file " + fullFileName);
		}
		file.write("TDHF0001", 8);
		isJustCreated = true;
	}
	if (!isJustCreated) {
		std::string fullFileName = _path + _historyFile.name;
		file.open(fullFileName, std::ios::out | std::ios::app | std::ios::binary);
		if (file.rdstate()) {
			ExitMsg("Error opening file " + fullFileName);
		}
	}
	
	file.write(reinterpret_cast<const char*>(_historyFile.buffer.buffer.data()), _historyFile.buffer.buffer.size());
	file.close();
	_historyFile.buffer.buffer.clear();
}

//===============================================================================
//
//===============================================================================

void DbSerializer::Update(double dt)
{
	_historyFile.timeToFlush -= dt;
	if (_historyFile.timeToFlush <= 0) {
		HistoryFlush();
	}
}


//===============================================================================
// 
//===============================================================================

void DbSerializer::SetPath(const std::string& path)
{
	_path = path;
}

//===============================================================================
// Записывает всю базу в файл
// Имя файла базы конструирует из имени базы
//===============================================================================

void DbSerializer::SaveDatabase()
{
	HistoryFlush();            // Флаш истории
	_historyFile.name.clear(); // Сбросить имя (признак, что файл истории для последнего файла базы ещё не создавался и его нужно будет создать)

	std::string timestamp = std::to_string(std::time(0));
	std::string fileName = "TextsBase_" + _pDataBase->_dbName + "_" + timestamp + ".bin";
	std::string fullFileName = _path + fileName;

	std::ofstream file(fullFileName, std::ios::out | std::ios::app | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error creating file " + fullFileName);
	}

	SerializationBuffer buffer;
	buffer.PushStringWithoutZero("TDBF0001");

	buffer.Push(_pDataBase->_newAttributeId);
	buffer.Push(_pDataBase->_attributeProps.size());
	for (const auto& atribProp : _pDataBase->_attributeProps) {
		atribProp.SaveToBase(buffer);
	}

	buffer.Push(_pDataBase->_newFolderId);
	buffer.Push(_pDataBase->_folders.size());
	for (const auto& folder : _pDataBase->_folders) {
		folder.SaveToBase(buffer);
	}

	file.write(reinterpret_cast<const char*>(buffer.buffer.data()), buffer.buffer.size());
	file.close();
}

//===============================================================================
// 
//===============================================================================

std::string DbSerializer::FindFreshBaseFileName(uint32_t& resultTimestamp)
{
	namespace fs = std::experimental::filesystem;

	uint32_t maxTimestamp = 0;
	std::string resultFileName;

	std::string prefix = "TextsBase_";
	std::string combinedPrefix = prefix + _pDataBase->_dbName;

	for (auto& p : fs::directory_iterator(_path)) {
		std::string fullFileName = p.path().string();
		std::string fileName = fullFileName.c_str() + _path.length();
		if (fileName.compare(0, combinedPrefix.length(), combinedPrefix)) {
			continue;
		}
		int offs = fileName.length() - 14;
		std::string timestampStr = fileName.substr(offs, 10);
		uint32_t timestamp = stoi(timestampStr);
		if (timestamp > maxTimestamp) {
			maxTimestamp = timestamp;
			resultFileName = fileName;
		}
	}
	resultTimestamp = maxTimestamp;
	return resultFileName;
}

//===============================================================================
// 
//===============================================================================

std::string DbSerializer::FindHistoryFileName(uint32_t tsBaseFile)
{
	namespace fs = std::experimental::filesystem;

	uint32_t tsFound = 0;
	std::string resultFileName;

	std::string prefix = "TextsHistory_";
	std::string combinedPrefix = prefix + _pDataBase->_dbName;

	for (auto& p : fs::directory_iterator(_path)) {
		std::string fullFileName = p.path().string();
		std::string fileName = fullFileName.c_str() + _path.length();

		if (fileName.compare(0, combinedPrefix.length(), combinedPrefix)) {
			continue;
		}
		int offs = fileName.length() - 14;
		std::string timestampStr = fileName.substr(offs, 10);
		uint32_t ts = stoi(timestampStr);
		if (ts < tsBaseFile) {
			continue;
		}
		if (tsFound) {
			ExitMsg("More than one HistoryFile per BaseFile");
		}
		tsFound = ts;
		resultFileName = fileName;
	}
	return resultFileName;
}

//===============================================================================
// 
//===============================================================================

void DbSerializer::LoadDatabaseInner(const std::string& fullFileName)
{
	uint32_t fileSize = 0;
	{
		std::ifstream file(fullFileName, std::ios::binary | std::ios::ate);
		if (file.rdstate()) {
			ExitMsg("Error opening file " + fullFileName);
		}
		fileSize = static_cast<uint32_t>(file.tellg());
	}

	DeserializationBuffer deserialBuf;
	deserialBuf.buffer.resize(fileSize + 1);  // +1 потому, что строки грузим путём временного добавления 0 в конце
	deserialBuf.buffer[fileSize] = 0;

	std::ifstream file(fullFileName, std::ios::in | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error opening file " + fullFileName);
	}
	file.read(reinterpret_cast<char*>(deserialBuf.buffer.data()), fileSize);
	file.close();

	deserialBuf.offset = 8; // Пропускаем сигнатуру и номер версии
	std::vector<uint8_t> attributesIdToType; // Для быстрой перекодировки id атрибута в его type

	_pDataBase->_newAttributeId = deserialBuf.GetUint<uint8_t>();
	uint32_t attributesNum = deserialBuf.GetUint<uint32_t>();
	for (uint32_t i = 0; i < attributesNum; ++i) {
		_pDataBase->_attributeProps.emplace_back(deserialBuf);
		uint8_t id = _pDataBase->_attributeProps.back().id;
		if (id >= attributesIdToType.size()) {
			attributesIdToType.resize(id + 1);
		}
		attributesIdToType[id] = _pDataBase->_attributeProps.back().type;
	}

	_pDataBase->_newFolderId = deserialBuf.GetUint<uint32_t>();
	uint32_t foldersNum = deserialBuf.GetUint<uint32_t>();
	for (uint32_t i = 0; i < foldersNum; ++i) {
		_pDataBase->_folders.emplace_back();
		_pDataBase->_folders.back().CreateFromBase(deserialBuf, attributesIdToType);
	}
}

//===============================================================================
// 
//===============================================================================

void DbSerializer::LoadHistoryInner(const std::string& fullFileName)
{
	uint32_t fileSize = 0;
	{
		std::ifstream file(fullFileName, std::ios::binary | std::ios::ate);
		if (file.rdstate()) {
			ExitMsg("Error opening file " + fullFileName);
		}
		fileSize = static_cast<uint32_t>(file.tellg());
	}

	DeserializationBuffer deserialBuf;
	deserialBuf.buffer.resize(fileSize + 1);  // +1 потому, что строки грузим путём временного добавления 0 в конце
	deserialBuf.buffer[fileSize] = 0;

	std::ifstream file(fullFileName, std::ios::in | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error opening file " + fullFileName);
	}
	file.read(reinterpret_cast<char*>(deserialBuf.buffer.data()), fileSize);
	file.close();

	deserialBuf.offset = 8; // Пропускаем сигнатуру и номер версии

	while (true)
	{
		std::string modifierLogin;
		deserialBuf.GetString<uint8_t>(modifierLogin);
		uint8_t actionType = deserialBuf.GetUint<uint8_t>();
		switch (actionType)
		{
		case ActionCreateFolder:
			_pDataBase->_folders.emplace_back();
			_pDataBase->_folders.back().CreateFromHistory(deserialBuf);
			_pDataBase->_newFolderId = _pDataBase->_folders.back().id + 1;
			break;
		default:
			ExitMsg("Unknown action type");
			break;
		}
		if (deserialBuf.IsEmpty()) {
			break;
		}
	}
}

//===============================================================================
// Читает базу из полного файла и файла истории
// Имена файлов базы и истории конструирует из имени базы, выбирает самые свежие файлы
//===============================================================================

void DbSerializer::LoadDatabaseAndHistory()
{
	// === Прочитать файл основной базы ==============================

	uint32_t baseTimestamp = 0;
	std::string fileName = FindFreshBaseFileName(baseTimestamp);
	if (fileName.empty()) {
		return;
	}
	LoadDatabaseInner(_path + fileName);

	// === Прочитать файл истории ============================

	_historyFile.name = FindHistoryFileName(baseTimestamp);
	if (_historyFile.name.empty()) {
		return;
	}
	LoadHistoryInner(_path + _historyFile.name);
}

//===============================================================================
//
//===============================================================================

void DbSerializer::PushCommonHeader(uint32_t timestamp, const std::string& loginOfLastModifier, uint8_t actionType)
{
	_historyFile.buffer.PushStringWithoutZero<uint8_t>(loginOfLastModifier);
	_historyFile.buffer.Push(actionType);
	_historyFile.buffer.Push(timestamp);
}

//===============================================================================
//
//===============================================================================

void DbSerializer::PushCreateFolder(const Folder& folder, const std::string& loginOfLastModifier)
{
	PushCommonHeader(folder.timestampCreated, loginOfLastModifier, ActionCreateFolder);
	_historyFile.buffer.Push(folder.id);
	_historyFile.buffer.Push(folder.parentId);
	_historyFile.buffer.PushStringWithoutZero<uint8_t>(folder.name);
}

//===============================================================================
//
//===============================================================================

AttributeProperty::AttributeProperty(DeserializationBuffer& buffer)
{
	id = buffer.GetUint<uint8_t>();
	visiblePosition = buffer.GetUint<uint8_t>();
	isVisible = static_cast<bool>(buffer.GetUint<uint8_t>());
	timestampCreated = buffer.GetUint<uint32_t>();
	buffer.GetString<uint8_t>(name);
	type = buffer.GetUint<uint8_t>();
	param1 = buffer.GetUint<uint32_t>();
	param2 = buffer.GetUint<uint32_t>();
}

//===============================================================================
//
//===============================================================================

void AttributeProperty::SaveToBase(SerializationBuffer& buffer) const
{
	buffer.Push(id);
	buffer.Push(visiblePosition);
	uint8_t flag = isVisible;
	buffer.Push(flag);
	buffer.Push(timestampCreated);
	buffer.PushStringWithoutZero<uint8_t>(name);
	buffer.Push(type);
	buffer.Push(param1);
	buffer.Push(param2);
}

//===============================================================================
//
//===============================================================================

void Folder::CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	id = buffer.GetUint<uint32_t>();
	timestampCreated = buffer.GetUint<uint32_t>();
	timestampModified = buffer.GetUint<uint32_t>();
	buffer.GetString<uint16_t>(name);
	parentId = buffer.GetUint<uint32_t>();
	uint32_t textsNum = buffer.GetUint<uint32_t>();
	for (uint32_t i = 0; i < textsNum; ++i) {
		texts.emplace_back(std::make_shared<TextTranslated>(buffer, attributesIdToType));
	}
}

//===============================================================================
//
//===============================================================================

void Folder::CreateFromHistory(DeserializationBuffer& buffer)
{
	uint32_t timestamp = buffer.GetUint<uint32_t>();
	timestampCreated = timestamp;
	timestampModified = timestamp;
	id = buffer.GetUint<uint32_t>();
	parentId = buffer.GetUint<uint32_t>();
	buffer.GetString<uint8_t>(name);
}

//===============================================================================
//
//===============================================================================

void Folder::CreateFromPacket(DeserializationBuffer& buffer)
{
	timestampCreated = static_cast<uint32_t>(std::time(0));
	timestampModified = timestampCreated;
	parentId = buffer.GetUint<uint32_t>();
	buffer.GetString<uint8_t>(name);
}

//===============================================================================
//
//===============================================================================

void Folder::SaveToBase(SerializationBuffer& buffer) const
{
	buffer.Push(id);
	buffer.Push(timestampCreated);
	buffer.Push(timestampModified);
	buffer.PushStringWithoutZero<uint16_t>(name);
	buffer.Push(parentId);
	buffer.Push(texts.size());
	for (const auto& text : texts) {
		text->SaveToBase(buffer);
	}
}

//===============================================================================
//
//===============================================================================

TextTranslated::TextTranslated(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	buffer.GetString<uint8_t>(id);
	timestampCreated = buffer.GetUint<uint32_t>();
	timestampModified = buffer.GetUint<uint32_t>();
	buffer.GetString<uint8_t>(loginOfLastModifier);
	offsLastModified = buffer.GetUint<uint32_t>();
	buffer.GetString<uint16_t>(baseText);
	uint8_t attributesNum = buffer.GetUint<uint8_t>();
	for (uint8_t i = 0; i < attributesNum; ++i) {
		attributes.emplace_back(buffer, attributesIdToType);
	}
}

//===============================================================================
//
//===============================================================================

void TextTranslated::SaveToBase(SerializationBuffer& buffer) const
{
	buffer.PushStringWithoutZero<uint8_t>(id);
	buffer.Push(timestampCreated);
	buffer.Push(timestampModified);
	buffer.PushStringWithoutZero<uint8_t>(loginOfLastModifier);
	buffer.Push(offsLastModified);
	buffer.PushStringWithoutZero<uint16_t>(baseText);
	uint8_t attributesNum = static_cast<uint8_t>(attributes.size());
	buffer.Push(attributesNum);
	for (const auto& attrib : attributes) {
		attrib.SaveToBase(buffer);
	}
}

//===============================================================================
//
//===============================================================================

AttributeInText::AttributeInText(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	id = buffer.GetUint<uint8_t>();
	if (id >= attributesIdToType.size()) {
		ExitMsg("id >= attributesIdToType.size()");
	}
	type = attributesIdToType[id];
	switch (type) {
		case AttributeProperty::Translation_t:
		case AttributeProperty::CommonText_t:
			{
				buffer.GetString<uint16_t>(text);
			}
			break;
		case AttributeProperty::Checkbox_t:
			flagState = buffer.GetUint<uint8_t>();
			break;
	
		default:
			ExitMsg("Wrong attribute type!");
	}
}

//===============================================================================
//
//===============================================================================

void AttributeInText::SaveToBase(SerializationBuffer& buffer) const
{
	buffer.Push(id);
	switch (type) {
		case AttributeProperty::Translation_t:
		case AttributeProperty::CommonText_t:
			{
				buffer.PushStringWithoutZero<uint16_t>(text);
			}
			break;
		case AttributeProperty::Checkbox_t:
			buffer.Push(flagState);
			break;
	
		default:
			ExitMsg("Wrong attribute type!");
	}
}

//===============================================================================
//
//===============================================================================

void STextsToolApp::Update(double dt)
{
	_messagesMgr.Update(dt);
	for (const auto& db : _dbs) {
		db->Update(dt);
	}
}

//===============================================================================
//
//===============================================================================

STextsToolApp::STextsToolApp(): _messagesMgr(this)
{
}

//===============================================================================
//
//===============================================================================

TextsDatabase::Ptr SClientMessagesMgr::GetDbPtrByDbName(const std::string& dbName)
{
	for (const auto& db : _app->_dbs) {
		if (dbName == db->_dbName) {
			return db;
		}
	}
	ExitMsg("Database not found: " + dbName);  
	return nullptr;
}

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::Update(double dt)
{
	for (auto& client: _app->_clients) {
		// !!! Вероятно, тут надо проверить наличие базы и если нету, то запустить фоновую загрузку, а выполнение запроса отложить
		TextsDatabase::Ptr db = GetDbPtrByDbName(client->_dbName);
		for (const auto& el : client->_msgsQueueIn) {
			switch (el->actionType) {
			case DbSerializer::ActionCreateFolder:
			{
				db->_folders.emplace_back();
				db->_folders.back().CreateFromPacket(el->_msgData);
				db->_folders.back().id = db->_newFolderId++;


			}
			break;
			default:
				ExitMsg("Wrong actionType");
			}
		}
		client->_msgsQueueIn.resize(0); // Не используем clear, чтобы гарантировать отсутствие релокации
	}
}

//===============================================================================
//
//===============================================================================

SClientMessagesMgr::SClientMessagesMgr(STextsToolApp* app): _app(app)
{
}