#include <experimental/filesystem>

#include "DbSerializer.h"
#include "../SharedSrc/Shared.h"


void ExitMsg(const std::string& message);


//===============================================================================

DbSerializer::DbSerializer(TextsDatabase* pDataBase) : _pDataBase(pDataBase)
{
}

//===============================================================================

void DbSerializer::SetPath(const std::string& path)
{
	_path = path;
}

//===============================================================================

void DbSerializer::SaveDatabase()
{
	std::string timestamp = std::to_string(std::time(0));
	std::string fileName = "TextsBase_" + _pDataBase->_dbName + "_" + timestamp + ".bin";
	std::string fullFileName = _path + fileName;

	std::ofstream file(fullFileName, std::ios::out | std::ios::trunc | std::ios::binary);
	if (file.rdstate()) {
		ExitMsg("Error creating file " + fullFileName);
	}

	SerializationBuffer buffer;
	buffer.PushStringWithoutZero("TDBF0001");

//	buffer.PushUint8(_pDataBase->_newAttributeId);
	buffer.PushUint32(_pDataBase->_attributeProps.size());
	for (const auto& atribProp : _pDataBase->_attributeProps) {
		atribProp.SaveFullDump(buffer);
	}

//	buffer.PushUint32(_pDataBase->_newFolderId);
	buffer.PushUint32(_pDataBase->_folders.size());
	for (const auto& folder : _pDataBase->_folders) {
		folder.SaveFullDump(buffer);
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

//	_pDataBase->_newAttributeId = buf.GetUint8();
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

//	_pDataBase->_newFolderId = buf.GetUint32();
	uint32_t foldersNum = buf.GetUint32();
	for (uint32_t i = 0; i < foldersNum; ++i) {
		_pDataBase->_folders.emplace_back();
		_pDataBase->_folders.back().LoadFullDump(buf, attributesIdToType);
	}
}

//===============================================================================

bool DbSerializer::LoadDatabase()
{
	// === Прочитать файл основной базы ==============================

	uint32_t baseTimestamp = 0;
	std::string fileName = FindFreshBaseFileName(baseTimestamp);
	if (fileName.empty()) {
		return false;
	}
	LoadDatabaseInner(_path + fileName);

	return true;
}

