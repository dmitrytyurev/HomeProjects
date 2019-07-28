#include "pch.h"

#include <experimental/filesystem>

#include "DbSerializer.h"


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
// ���������� ��� ���� � ����
// ��� ����� ���� ������������ �� ����� ����
//===============================================================================

void DbSerializer::SaveDatabase()
{
	HistoryFlush();            // ���� �������
	_historyFile.name.clear(); // �������� ��� (�������, ��� ���� ������� ��� ���������� ����� ���� ��� �� ���������� � ��� ����� ����� �������)

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
	buf._buffer.resize(fileSize + 1);  // +1 ������, ��� ������ ������ ���� ���������� ���������� 0 � �����
	buf._buffer[fileSize] = 0;

	std::ifstream file(fullFileName, std::ios::in | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error opening file " + fullFileName);
	}
	file.read(reinterpret_cast<char*>(buf._buffer.data()), fileSize);
	file.close();

	buf.offset = 8; // ���������� ��������� � ����� ������
	std::vector<uint8_t> attributesIdToType; // ��� ������� ������������� id �������� � ��� type

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
	buf._buffer.resize(fileSize + 1);  // +1 ������, ��� ������ ������ ���� ���������� ���������� 0 � �����
	buf._buffer[fileSize] = 0;

	std::ifstream file(fullFileName, std::ios::in | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error opening file " + fullFileName);
	}
	file.read(reinterpret_cast<char*>(buf._buffer.data()), fileSize);
	file.close();

	buf.offset = 8; // ���������� ��������� � ����� ������

	while (true)
	{
		std::string modifierLogin;
		buf.GetString<uint8_t>(modifierLogin);
		uint8_t actionType = buf.GetUint<uint8_t>();
		switch (actionType)
		{
		case ActionCreateFolder:
			_pDataBase->_folders.emplace_back();
			_pDataBase->_folders.back().CreateFromHistory(buf);
			_pDataBase->_newFolderId = _pDataBase->_folders.back().id + 1;
			break;
		case ActionDeleteFolder:
		{
			uint32_t folderId = buf.GetUint<uint32_t>();
			auto& f = _pDataBase->_folders;
			auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
			if (result != std::end(f)) {
				f.erase(result);
			} else {
				ExitMsg("LoadFromHistory: ActionDeleteFolder: Folder id not found");
			}
		}
		break;
		case ActionChangeFolderParent:
		{
			uint32_t folderId = buf.GetUint<uint32_t>();
			uint32_t newParentFolderId = buf.GetUint<uint32_t>();
			auto& f = _pDataBase->_folders;
			auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
			if (result != std::end(f)) {
				result->parentId = newParentFolderId;
			} else {
				ExitMsg("LoadFromHistory: ActionChangeFolderParent: Folder id not found");
			}
		}
		break;
		default:
			ExitMsg("Unknown action type");
			break;
		}
		if (buf.IsEmpty()) {
			break;
		}
	}
}

//===============================================================================
// ������ ���� �� ������� ����� � ����� �������
// ����� ������ ���� � ������� ������������ �� ����� ����, �������� ����� ������ �����
//===============================================================================

void DbSerializer::LoadDatabaseAndHistory()
{
	// === ��������� ���� �������� ���� ==============================

	uint32_t baseTimestamp = 0;
	std::string fileName = FindFreshBaseFileName(baseTimestamp);
	if (fileName.empty()) {
		return;
	}
	LoadDatabaseInner(_path + fileName);

	// === ��������� ���� ������� ============================

	_historyFile.name = FindHistoryFileName(baseTimestamp);
	if (_historyFile.name.empty()) {
		return;
	}
	LoadHistoryInner(_path + _historyFile.name);
}

//===============================================================================
//
//===============================================================================

void DbSerializer::PushHeader(SerializationBuffer& buffer, uint32_t timestamp, const std::string& loginOfLastModifier, uint8_t actionType)
{
	buffer.PushStringWithoutZero<uint8_t>(loginOfLastModifier);
	buffer.Push(actionType);
	buffer.Push(timestamp);
}

