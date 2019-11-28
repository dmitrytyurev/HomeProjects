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
	std::string fileName = _pDataBase->_dbName + ".bin";
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

void DbSerializer::LoadDatabase()
{
	std::string fullFileName = _path + _pDataBase->_dbName + ".bin";
	uint32_t fileSize = 0;
	{
		std::ifstream file(fullFileName, std::ios::binary | std::ios::ate);
		if (file.rdstate()) {
			return;
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

