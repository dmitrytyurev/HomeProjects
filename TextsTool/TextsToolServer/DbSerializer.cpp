#include <experimental/filesystem>

#include "DbSerializer.h"
#include "Common.h"
#include "../SharedSrc/Shared.h"


void ExitMsg(const std::string& message);


//===============================================================================

DbSerializer::DbSerializer(TextsDatabase* pDataBase) : _pDataBase(pDataBase)
{
}

//===============================================================================

void DbSerializer::HistoryFlush()
{
	const double HISTORY_FLUSH_INTERVAL = 1.f;

	_historyFile.timeToFlush = HISTORY_FLUSH_INTERVAL;

	if (_historyFile.buffer.buffer.empty()) {
		return;
	}


	std::ofstream file;

	if (_historyFile.name.empty()) {
		std::string timestamp = std::to_string(std::time(0));
		_historyFile.name = "TextsHistory_" + _pDataBase->_dbName + "_" + timestamp + ".bin";
		std::string fullFileName = _path + _historyFile.name;
		file.open(fullFileName, std::ios::out | std::ios::trunc | std::ios::binary);
		if (file.rdstate()) {
			ExitMsg("Error creating file " + fullFileName);
		}
		const int HEADER_SIZE = 8;
		file.write("TDHF0001", HEADER_SIZE);
		_historyFile.savedFileSize = HEADER_SIZE;
	}
	else {
		std::string fullFileName = _path + _historyFile.name;
		file.open(fullFileName, std::ios::out | std::ios::app | std::ios::binary);
		if (file.rdstate()) {
			ExitMsg("Error opening file " + fullFileName);
		}
	}

	file.write(reinterpret_cast<const char*>(_historyFile.buffer.GetData()), _historyFile.buffer.GetSize());
	file.close();
	_historyFile.savedFileSize += _historyFile.buffer.GetSize();
	_historyFile.buffer.buffer.clear();
}

//===============================================================================

SerializationBuffer& DbSerializer::GetHistoryBuffer()
{
	return _historyFile.buffer;
}

//===============================================================================

void DbSerializer::Update(double dt)
{
	_historyFile.timeToFlush -= dt;
	if (_historyFile.timeToFlush <= 0) {
		HistoryFlush();
	}
}


//===============================================================================

void DbSerializer::SetPath(const std::string& path)
{
	_path = path;
}

//===============================================================================

void DbSerializer::SaveDatabase()
{
	HistoryFlush();            // Флаш истории
	_historyFile.name.clear(); // Сбросить имя (признак, что файл истории для последнего файла базы ещё не создавался и его нужно будет создать)

	std::string timestamp = std::to_string(std::time(0));
	std::string fileName = "TextsBase_" + _pDataBase->_dbName + "_" + timestamp + ".bin";
	std::string fullFileName = _path + fileName;

	std::ofstream file(fullFileName, std::ios::out | std::ios::trunc | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error creating file " + fullFileName);
	}

	SerializationBuffer buffer;
	buffer.PushStringWithoutZero("TDBF0001");

	buffer.PushUint8(_pDataBase->_newAttributeId);
	buffer.PushUint32(_pDataBase->_attributeProps.size());
	for (const auto& atribProp : _pDataBase->_attributeProps) {
		atribProp.SaveFullDump(buffer);
	}

	buffer.PushUint32(_pDataBase->_newFolderId);
	buffer.PushUint32(_pDataBase->_folders.size());
	for (const auto& folder : _pDataBase->_folders) {
		folder.SaveFullDump(buffer, false);
	}

	file.write(reinterpret_cast<const char*>(buffer.GetData()), buffer.GetSize());
	file.close();
}

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

	DeserializationBuffer buf;
	buf._buffer.resize(fileSize);

	std::ifstream file(fullFileName, std::ios::in | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error opening file " + fullFileName);
	}
	file.read(reinterpret_cast<char*>(buf._buffer.data()), fileSize);
	file.close();

	buf.offset = 8; // Пропускаем сигнатуру и номер версии
	std::vector<uint8_t> attributesIdToType; // Для быстрой перекодировки id атрибута в его type

	_pDataBase->_newAttributeId = buf.GetUint8();
	uint32_t attributesNum = buf.GetUint32();
	for (uint32_t i = 0; i < attributesNum; ++i) {
		_pDataBase->_attributeProps.emplace_back();
		_pDataBase->_attributeProps.back().LoadFullDump(buf);
		uint8_t id = _pDataBase->_attributeProps.back().id;
		if (id >= attributesIdToType.size()) {
			attributesIdToType.resize(id + 1);
		}
		attributesIdToType[id] = _pDataBase->_attributeProps.back().type;
	}

	_pDataBase->_newFolderId = buf.GetUint32();
	uint32_t foldersNum = buf.GetUint32();
	for (uint32_t i = 0; i < foldersNum; ++i) {
		_pDataBase->_folders.emplace_back();
		_pDataBase->_folders.back().LoadFullDump(buf, attributesIdToType);
	}
}

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

	DeserializationBuffer buf;
	buf._buffer.resize(fileSize);

	std::ifstream file(fullFileName, std::ios::in | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error opening file " + fullFileName);
	}
	file.read(reinterpret_cast<char*>(buf._buffer.data()), fileSize);
	file.close();

	buf.offset = 8; // Пропускаем сигнатуру и номер версии

	while (true)
	{
		if (buf.GetRestBytesNum() == 0) {
			break;
		}
		uint32_t offsToEventBegin = buf.offset;
		std::string modifierLogin;
		buf.GetString8(modifierLogin);
		uint32_t ts = buf.GetUint32();
		uint8_t actionType = buf.GetUint8();
		switch (actionType)
		{
		case EventType::CreateFolder:
			_pDataBase->_folders.emplace_back();
			_pDataBase->_folders.back().CreateFromHistory(buf, ts);
			_pDataBase->_newFolderId = _pDataBase->_folders.back().id + 1;
			break;
		case EventType::DeleteFolder:
			SClientMessagesMgr::ModifyDbDeleteFolder(buf, *_pDataBase);
		break;
		case EventType::ChangeFolderParent:
			SClientMessagesMgr::ModifyDbChangeFolderParent(buf, *_pDataBase, ts);
			break;
		case EventType::RenameFolder:
			SClientMessagesMgr::ModifyDbRenameFolder(buf, *_pDataBase, ts);
			break;
		case EventType::CreateAttribute:
			_pDataBase->_attributeProps.emplace_back();
			_pDataBase->_attributeProps.back().CreateFromHistory(buf, ts);
			_pDataBase->_newAttributeId = _pDataBase->_attributeProps.back().id + 1;
			break;
		case EventType::DeleteAttribute:
			SClientMessagesMgr::ModifyDbDeleteAttribute(buf, *_pDataBase, ts);
			break;
		case EventType::RenameAttribute:
			SClientMessagesMgr::ModifyDbRenameAttribute(buf, *_pDataBase);
			break;
		case EventType::ChangeAttributeVis:
			SClientMessagesMgr::ModifyDbChangeAttributeVis(buf, *_pDataBase);
			break;
		case EventType::CreateText:
		{
			uint32_t folderId = buf.GetUint32();
			std::string textId;
			buf.GetString8(textId);
			auto& f = _pDataBase->_folders;
			auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
			if (result != std::end(f)) {
				result->texts.emplace_back();
				TextTranslated& tt = *(result->texts.back());
				tt.id = textId;
				tt.timestampCreated = ts;
				tt.timestampModified = ts;
				tt.loginOfLastModifier = modifierLogin;
				tt.offsLastModified = offsToEventBegin;
			}
			else {
				ExitMsg("DbSerializer::LoadHistoryInner: EventCreateText: folder not found");
			}
		}
		break;
		case EventType::DeleteText:
		{
			uint32_t prevTsModified = 0;     // Модифицируем базу
			uint32_t prevOffsModified = 0;
			SClientMessagesMgr::ModifyDbDeleteText(buf, *_pDataBase, prevTsModified, prevOffsModified);
			buf.offset += sizeof(uint32_t) + sizeof(uint32_t); // Пропускаем ts предыдущего изменения текста текста (для поиска файла истории с инфой о нём) и смещение в файле истории до него
		}
		break;
		case EventType::MoveTextToFolder:
		{
			uint32_t prevTsModified = 0;    // Модифицируем базу
			uint32_t prevOffsModified = 0;
			SClientMessagesMgr::ModifyDbMoveTextToFolder(buf, *_pDataBase, modifierLogin, ts, offsToEventBegin, prevTsModified, prevOffsModified);
			buf.offset += sizeof(uint32_t) + sizeof(uint32_t); // Пропускаем ts предыдущего изменения текста текста (для поиска файла истории с инфой о нём) и смещение в файле истории до него
		}
		break;
		case EventType::ChangeBaseText:
		{
			uint32_t prevTsModified = 0;   // Модифицируем базу
			uint32_t prevOffsModified = 0;
			SClientMessagesMgr::ModifyDbChangeBaseText(buf, *_pDataBase, modifierLogin, ts, offsToEventBegin, prevTsModified, prevOffsModified);
			buf.offset += sizeof(uint32_t) + sizeof(uint32_t); // Пропускаем ts предыдущего изменения текста текста (для поиска файла истории с инфой о нём) и смещение в файле истории до него
		}
		break;
		case EventType::ChangeAttributeInText:
		{
			buf.offset += sizeof(uint32_t) + sizeof(uint32_t); // Пропускаем ts предыдущего изменения текста текста (для поиска файла истории с инфой о нём) и смещение в файле истории до него
			uint32_t prevTsModified = 0;  // Модифицируем базу
			uint32_t prevOffsModified = 0;
			SClientMessagesMgr::ModifyDbChangeAttributeInText(buf, *_pDataBase, modifierLogin, ts, offsToEventBegin, prevTsModified, prevOffsModified);
		}
		break;
			
		// Важно: после чтения любого изменения текста, у него нужно заполнить следующие поля:
		//   timestampModified = ts;
		//   loginOfLastModifier = modifierLogin;
		//   offsLastModified = offsToEventBegin;
		// А у его каталога  (только непосредственно, без родителей) сделать timestampModified = ts

		default:
			ExitMsg("DbSerializer::LoadHistoryInner: Unknown action type");
			break;
		}
	}
}

//===============================================================================

bool DbSerializer::LoadDatabaseAndHistory()
{
	// === Прочитать файл основной базы ==============================

	uint32_t baseTimestamp = 0;
	std::string fileName = FindFreshBaseFileName(baseTimestamp);
	if (fileName.empty()) {
		return false;
	}
	LoadDatabaseInner(_path + fileName);

	// === Прочитать файл истории ============================

	_historyFile.name = FindHistoryFileName(baseTimestamp);
	if (_historyFile.name.empty()) {
		return true;
	}
	LoadHistoryInner(_path + _historyFile.name);
	return true;
}

//===============================================================================

uint32_t DbSerializer::GetCurrentPosInHistoryFile()
{
	return _historyFile.savedFileSize + _historyFile.buffer.GetSize();
}


