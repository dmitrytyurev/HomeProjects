#include "pch.h"

#include <experimental/filesystem>

#include "DbSerializer.h"
#include "Common.h"


void ExitMsg(const std::string& message);


//===============================================================================
//
//===============================================================================

DbSerializer::DbSerializer(TextsDatabase* pDataBase) : _pDataBase(pDataBase)
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
		const int HEADER_SIZE = 8;
		file.write("TDHF0001", HEADER_SIZE);
		isJustCreated = true;
		_historyFile.savedFileSize = HEADER_SIZE;
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
	_historyFile.savedFileSize += _historyFile.buffer.buffer.size();
	_historyFile.buffer.buffer.clear();
}

//===============================================================================
//
//===============================================================================

SerializationBuffer& DbSerializer::GetHistoryBuffer()
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

	DeserializationBuffer buf;
	buf._buffer.resize(fileSize + 1);  // +1 потому, что строки грузим путём временного добавления 0 в конце
	buf._buffer[fileSize] = 0;

	std::ifstream file(fullFileName, std::ios::in | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error opening file " + fullFileName);
	}
	file.read(reinterpret_cast<char*>(buf._buffer.data()), fileSize);
	file.close();

	buf.offset = 8; // Пропускаем сигнатуру и номер версии
	std::vector<uint8_t> attributesIdToType; // Для быстрой перекодировки id атрибута в его type

	_pDataBase->_newAttributeId = buf.GetUint<uint8_t>();
	uint32_t attributesNum = buf.GetUint<uint32_t>();
	for (uint32_t i = 0; i < attributesNum; ++i) {
		_pDataBase->_attributeProps.emplace_back();
		_pDataBase->_attributeProps.back().CreateFromBase(buf);
		uint8_t id = _pDataBase->_attributeProps.back().id;
		if (id >= attributesIdToType.size()) {
			attributesIdToType.resize(id + 1);
		}
		attributesIdToType[id] = _pDataBase->_attributeProps.back().type;
	}

	_pDataBase->_newFolderId = buf.GetUint<uint32_t>();
	uint32_t foldersNum = buf.GetUint<uint32_t>();
	for (uint32_t i = 0; i < foldersNum; ++i) {
		_pDataBase->_folders.emplace_back();
		_pDataBase->_folders.back().CreateFromBase(buf, attributesIdToType);
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

	DeserializationBuffer buf;
	buf._buffer.resize(fileSize + 1);  // +1 потому, что строки грузим путём временного добавления 0 в конце
	buf._buffer[fileSize] = 0;

	std::ifstream file(fullFileName, std::ios::in | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error opening file " + fullFileName);
	}
	file.read(reinterpret_cast<char*>(buf._buffer.data()), fileSize);
	file.close();

	buf.offset = 8; // Пропускаем сигнатуру и номер версии

	while (true)
	{
		uint32_t offsToEventBegin = buf.offset;
		std::string modifierLogin;
		buf.GetString<uint8_t>(modifierLogin);
		uint32_t ts = buf.GetUint<uint32_t>();
		uint8_t actionType = buf.GetUint<uint8_t>();
		switch (actionType)
		{
		case ActionCreateFolder:
			_pDataBase->_folders.emplace_back();
			_pDataBase->_folders.back().CreateFromHistory(buf, ts);
			_pDataBase->_newFolderId = _pDataBase->_folders.back().id + 1;
			break;
		case ActionDeleteFolder:
			SClientMessagesMgr::ModifyDbDeleteFolder(buf, *_pDataBase);
		break;
		case ActionChangeFolderParent:
			SClientMessagesMgr::ModifyDbChangeFolderParent(buf, *_pDataBase, ts);
			break;
		case ActionRenameFolder:
			SClientMessagesMgr::ModifyDbRenameFolder(buf, *_pDataBase, ts);
			break;
		case ActionCreateAttribute:
			_pDataBase->_attributeProps.emplace_back();
			_pDataBase->_attributeProps.back().CreateFromHistory(buf, ts);
			_pDataBase->_newAttributeId = _pDataBase->_attributeProps.back().id + 1;
			break;
		case ActionDeleteAttribute:
			SClientMessagesMgr::ModifyDbDeleteAttribute(buf, *_pDataBase, ts);
			break;
		case ActionRenameAttribute:
			SClientMessagesMgr::ModifyDbRenameAttribute(buf, *_pDataBase);
			break;
		case ActionChangeAttributeVis:
			SClientMessagesMgr::ModifyDbChangeAttributeVis(buf, *_pDataBase);
			break;
		case ActionCreateText:
		{
			uint32_t folderId = buf.GetUint<uint32_t>();
			std::string textId;
			buf.GetString<uint8_t>(textId);
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
				ExitMsg("DbSerializer::LoadHistoryInner: ActionCreateText: folder not found");
			}
		}
		break;
		case ActionDeleteText:
			SClientMessagesMgr::ModifyDbDeleteText(buf, *_pDataBase);
			break;
		case ActionMoveTextToFolder:
			SClientMessagesMgr::ModifyDbMoveTextToFolder(buf, *_pDataBase, modifierLogin, offsToEventBegin, ts);
			break;

			



		// !!!!!!!!!!!!!!!!
		// После чтения любого изменения текста, у него нужно заполнить следующие поля:
		//   timestampModified = tsModified;
		//   loginOfLastModifier = modifierLogin;
		//   offsLastModified = offsToEventBegin;

		default:
			ExitMsg("DbSerializer::LoadHistoryInner: Unknown action type");
			break;
		}
		if (buf.IsEmpty()) {
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
//===============================================================================

uint32_t DbSerializer::GetCurrentPosInHistoryFile()
{
	return _historyFile.savedFileSize + _historyFile.buffer.buffer.size();
}


