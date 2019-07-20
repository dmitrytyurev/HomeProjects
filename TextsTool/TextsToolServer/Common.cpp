#include "pch.h"

#include <iostream>
#include <ctime>
#include <iostream>
#include <fstream>
#include <experimental/filesystem>

#include "Common.h"


void ExitMsg(const std::string& message)
{
	std::cout << "Fatal: " << message << std::endl;
	exit(1);
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
// Загружает в объект базу из свежих файлов
//===============================================================================

TextsDatabase::TextsDatabase(const std::string dbName): _dbSerializer(this), _dbName(dbName)
{
}

//===============================================================================
//
//===============================================================================

void TextsDatabase::Update(float dt)
{

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

void DbSerializer::Update(float dt) // Как минимум для закрытия файла через 1 сек после записи
{
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
	std::string timestamp = std::to_string(std::time(0));
	std::string fileName = "TextsBase_" + _pDataBase->_dbName + "_" + timestamp + ".bin";
	std::string fullFileName = _path + fileName;

	std::ofstream myfile;
	myfile.open(fullFileName, std::ios::out | std::ios::app | std::ios::binary);
	if (myfile.rdstate()) {
		ExitMsg("Error creating file " + fileName);
	}

	SerializationBuffer serializationBuffer;
	serializationBuffer.PushStringWithoutZero("TDBF0001");

	serializationBuffer.Push(_pDataBase->_attributeProps.size());
	for (const auto& atribProp : _pDataBase->_attributeProps) {
		atribProp.serialize(serializationBuffer);
	}

	serializationBuffer.Push(_pDataBase->_folders.size());
	for (const auto& folder : _pDataBase->_folders) {
		folder.serialize(serializationBuffer);
	}

	myfile.write(reinterpret_cast<const char*>(serializationBuffer.buffer.data()), serializationBuffer.buffer.size());
	myfile.close();
}

//===============================================================================
// 
//===============================================================================

std::string DbSerializer::GetFreshBaseFileName()
{
	namespace fs = std::experimental::filesystem;

	uint32_t maxTimestamp = 0;
	for (auto& p : fs::directory_iterator(_path)) {
		std::string fullFileName = p.path().string();
		std::string fileName = fullFileName.c_str() + _path.length();

		if (fileName.compare(0, 9, "TextsBase") == 0) {
			std::string timestampStr = fileName.substr(17, 10);
			uint32_t timestamp = stoi(timestampStr);
			maxTimestamp = std::max(maxTimestamp, timestamp);
		}
	}

	if (maxTimestamp == 0) {
		ExitMsg("TextsBase_TestDB_??????????.bin not found");
	}

	std::string fileName = "TextsBase_" + _pDataBase->_dbName + "_" + std::to_string(maxTimestamp) + ".bin";
	return fileName;
}

//===============================================================================
// Читает базу из полного файла и файла истории
// Имена файлов базы и истории конструирует из имени базы, выбирает самые свежие файлы
//===============================================================================

void DbSerializer::LoadDatabaseAndHistory()
{
	std::string fileName = GetFreshBaseFileName();


	std::cout << fileName << '\n';

}

//===============================================================================
//
//===============================================================================

void AttributeProperty::serialize(SerializationBuffer& buffer) const
{
	buffer.Push(id);
	buffer.Push(visiblePosition);
	uint8_t flag = isVisible;
	buffer.Push(flag);
	buffer.Push(timestampCreated);
	uint8_t nameLength = static_cast<uint8_t>(name.length());
	buffer.Push(nameLength);
	buffer.PushStringWithoutZero(name);
	buffer.Push(type);
	buffer.Push(param1);
	buffer.Push(param2);
}

//===============================================================================
//
//===============================================================================

void Folder::serialize(SerializationBuffer& buffer) const
{
	buffer.Push(id);
	buffer.Push(timestampCreated);
	buffer.Push(timestampModified);
	uint16_t nameLength = static_cast<uint16_t>(name.length());
	buffer.Push(nameLength);
	buffer.PushStringWithoutZero(name);
	buffer.Push(parentId);
	buffer.Push(texts.size());
	for (const auto& text : texts) {
		text->serialize(buffer);
	}
}

//===============================================================================
//
//===============================================================================

void TextTranslated::serialize(SerializationBuffer& buffer) const
{
	uint8_t nameLength8 = static_cast<uint8_t>(id.length());
	buffer.Push(nameLength8);
	buffer.PushStringWithoutZero(id);
	buffer.Push(timestampCreated);
	buffer.Push(timestampModified);
	nameLength8 = static_cast<uint8_t>(loginOfLastModifier.length());
	buffer.Push(nameLength8);
	buffer.PushStringWithoutZero(loginOfLastModifier);
	buffer.Push(offsLastModified);
	uint16_t nameLength16 = static_cast<uint16_t>(baseText.length());
	buffer.Push(nameLength16);
	buffer.PushStringWithoutZero(baseText);
	uint8_t attributesNum = static_cast<uint8_t>(attributes.size());
	buffer.Push(attributesNum);
	for (const auto& attrib : attributes) {
		attrib.serialize(buffer);
	}
}


//===============================================================================
//
//===============================================================================

void AttributeInText::serialize(SerializationBuffer& buffer) const
{
	buffer.Push(id);
	switch (type) {
		case AttributeProperty::Translation_t:
		case AttributeProperty::CommonText_t:
			{
				uint16_t nameLength = static_cast<uint16_t>(text.length());
				buffer.Push(nameLength);
				buffer.PushStringWithoutZero(text);
			}
			break;
		case AttributeProperty::Checkbox_t:
			buffer.Push(flagState);
			break;
	
		default:
			ExitMsg("Wrong attribute type!");
	}
}
