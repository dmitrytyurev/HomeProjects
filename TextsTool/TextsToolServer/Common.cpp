#include "pch.h"

#include <iostream>
#include <ctime>
#include <iostream>
#include <fstream>

#include "Common.h"


void ExitMsg(const std::string& message)
{
	std::cout << "Fatal: " << message << std::endl;
	exit(1);
}

//===============================================================================
// ��������� � ������ ���� �� ������ ������
//===============================================================================

TextsDatabase::TextsDatabase(const std::string dbName): _dbSerializer(this), _dbName(dbName)
{
_dbSerializer.SaveDatabase();

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

void DbSerializer::Update(float dt) // ��� ������� ��� �������� ����� ����� 1 ��� ����� ������
{
}

//===============================================================================
// ������ ���� �� ������� ����� � ����� �������
// ����� ������ ���� � ������� ������������ �� ����� ����, �������� ����� ������ �����
//===============================================================================

void DbSerializer::LoadDatabaseAndHistory() 
{
}

//===============================================================================
// ���������� ��� ���� � ����
// ��� ����� ���� ������������ �� ����� ����
//===============================================================================

void DbSerializer::SaveDatabase()
{
	std::string timestamp = std::to_string(std::time(0));
	std::string fileName = "TextsBase_" + _pDataBase->_dbName + "_" + timestamp + ".bin";

	std::ofstream myfile;
	myfile.open(fileName, std::ios::out | std::ios::app | std::ios::binary);
	if (myfile.rdstate()) {
		ExitMsg("Error creating file " + fileName);
	}


	_serializationBuffer.PushStringWithoutZero("TDBF0001");


	myfile.write(reinterpret_cast<const char*>(_serializationBuffer.buffer.data()), _serializationBuffer.buffer.size());
	myfile.close();
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

uint32_t SerializationBuffer::GetCurrentOffset()
{

	return 0;  // !!! ����� ������ ���������� ������ �������! ��� ����� ������� � �����!!!
}

//===============================================================================
//
//===============================================================================

void SerializationBuffer::PushBytesFromOff(uint32_t offset, const void* bytes, int size)
{
	// !!! ��� ��� ����� ������? ��� � � ����� ����, � �� � ������?
}

