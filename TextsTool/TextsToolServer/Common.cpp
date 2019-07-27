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

SerializationBuffer& DbSerializer::GetSerialBuffer()
{
	return _historyFile.buffer;
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
		_pDataBase->_attributeProps.emplace_back();
		_pDataBase->_attributeProps.back().CreateFromBase(deserialBuf);
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

void DbSerializer::PushCommonHeader(SerializationBuffer& buffer, uint32_t timestamp, const std::string& loginOfLastModifier, uint8_t actionType)
{
	buffer.PushStringWithoutZero<uint8_t>(loginOfLastModifier);
	buffer.Push(actionType);
	buffer.Push(timestamp);
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
				Folder& folder = db->_folders.back();
				folder.CreateFromPacket(el->_msgData);
				folder.id = db->_newFolderId++;
				folder.SaveToHistory(db->GetSerialBuffer(), client->login);

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