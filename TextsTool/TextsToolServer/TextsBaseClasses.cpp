#include "pch.h"

#include <iostream>
#include <ctime>
#include <iostream>
#include <experimental/filesystem>

#include "Common.h"
#include "DbSerializer.h"

void ExitMsg(const std::string& message);


//===============================================================================
// Загружает в объект базу из свежих файлов
//===============================================================================

void TextsDatabase::CreateFromBase(const std::string path, const std::string dbName)
{
	_dbName = dbName;
	_dbSerializer = std::make_unique<DbSerializer>(this);
	_dbSerializer->SetPath(path);
	_dbSerializer->LoadDatabaseAndHistory();
}

//===============================================================================
//
//===============================================================================

void TextsDatabase::Update(double dt)
{
	_dbSerializer->Update(dt);
}

//===============================================================================
//
//===============================================================================

SerializationBuffer& TextsDatabase::GetHistoryBuffer()
{
	return _dbSerializer->GetHistoryBuffer();
}

//===============================================================================
//
//===============================================================================

uint32_t TextsDatabase::GetCurrentPosInHistoryFile()
{
	return _dbSerializer->GetCurrentPosInHistoryFile();
}

//===============================================================================
//
//===============================================================================

void AttributeProperty::CreateFromBase(DeserializationBuffer& buffer)
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
// Создание объекта из файла истории
//===============================================================================

void AttributeProperty::CreateFromHistory(DeserializationBuffer& buffer, uint32_t ts)
{
	id = buffer.GetUint<uint8_t>();
	visiblePosition = buffer.GetUint<uint8_t>();
	isVisible = static_cast<bool>(buffer.GetUint<uint8_t>());
	timestampCreated = ts;
	buffer.GetString<uint8_t>(name);
	type = buffer.GetUint<uint8_t>();
	param1 = buffer.GetUint<uint32_t>();
	param2 = buffer.GetUint<uint32_t>();
}

//===============================================================================
// Создание объекта из сообщения от клиента 
//===============================================================================
void AttributeProperty::CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId)
{
	id = newId;
	visiblePosition = buffer.GetUint<uint8_t>();
	isVisible = static_cast<bool>(buffer.GetUint<uint8_t>());
	timestampCreated = ts;
	buffer.GetString<uint8_t>(name);
	type = buffer.GetUint<uint8_t>();
	param1 = buffer.GetUint<uint32_t>();
	param2 = 0;
}

//===============================================================================
//
//===============================================================================

void AttributeProperty::SaveToHistory(TextsDatabasePtr db, const std::string& loginOfLastModifier) const
{
	auto& buffer = db->GetHistoryBuffer();
	buffer.PushStringWithoutZero<uint8_t>(loginOfLastModifier);
	buffer.Push(timestampCreated);
	buffer.Push(static_cast<uint8_t>(EventCreateAttribute));
	buffer.Push(id);
	buffer.Push(visiblePosition);
	buffer.Push(static_cast<uint8_t>(isVisible));
	buffer.PushStringWithoutZero<uint8_t>(name);
	buffer.Push(type);
	buffer.Push(param1);
	buffer.Push(param2);
}


//===============================================================================
//
//===============================================================================

SerializationBufferPtr AttributeProperty::SaveToPacket(const std::string& loginOfModifier) const
{
	auto bufPtr = std::make_shared<SerializationBuffer>();

	bufPtr->PushStringWithoutZero<uint8_t>(loginOfModifier);
	bufPtr->Push(timestampCreated);
	bufPtr->Push(static_cast<uint8_t>(EventCreateAttribute));
	bufPtr->Push(id);
	bufPtr->Push(visiblePosition);
	bufPtr->Push(static_cast<uint8_t>(isVisible));
	bufPtr->PushStringWithoutZero<uint8_t>(name);
	bufPtr->Push(type);
	bufPtr->Push(param1);
	return bufPtr;
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
		texts.emplace_back(std::make_shared<TextTranslated>());
		texts.back()->CreateFromBase(buffer, attributesIdToType);
	}
}

//===============================================================================
//
//===============================================================================

void Folder::CreateFromHistory(DeserializationBuffer& buffer, uint32_t ts)
{
	timestampCreated = ts;
	timestampModified = ts;
	id = buffer.GetUint<uint32_t>();
	parentId = buffer.GetUint<uint32_t>();
	buffer.GetString<uint8_t>(name);
}

//===============================================================================
//
//===============================================================================

void Folder::CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId)
{
	id = newId;
	timestampCreated = ts;
	timestampModified = ts;
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
	buffer.Push((uint32_t)texts.size());
	for (const auto& text : texts) {
		text->SaveToBase(buffer);
	}
}

//===============================================================================
//
//===============================================================================

void Folder::SaveToHistory(TextsDatabasePtr db, const std::string& loginOfLastModifier) const
{
	auto& buffer = db->GetHistoryBuffer();
	buffer.PushStringWithoutZero<uint8_t>(loginOfLastModifier);
	buffer.Push(timestampCreated);
	buffer.Push(static_cast<uint8_t>(EventCreateFolder));
	buffer.Push(id);
	buffer.Push(parentId);
	buffer.PushStringWithoutZero<uint8_t>(name);
}

//===============================================================================
//
//===============================================================================

SerializationBufferPtr Folder::SaveToPacket(const std::string& loginOfModifier) const
{
	auto bufPtr = std::make_shared<SerializationBuffer>();

	bufPtr->PushStringWithoutZero<uint8_t>(loginOfModifier);
	bufPtr->Push(timestampCreated);
	bufPtr->Push(static_cast<uint8_t>(EventCreateFolder));
	bufPtr->Push(id);
	bufPtr->Push(parentId);
	bufPtr->PushStringWithoutZero<uint8_t>(name);
	return bufPtr;
}

//===============================================================================
//
//===============================================================================

void TextTranslated::CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	buffer.GetString<uint8_t>(id);
	timestampCreated = buffer.GetUint<uint32_t>();
	timestampModified = buffer.GetUint<uint32_t>();
	buffer.GetString<uint8_t>(loginOfLastModifier);
	offsLastModified = buffer.GetUint<uint32_t>();
	buffer.GetString<uint16_t>(baseText);
	uint8_t attributesNum = buffer.GetUint<uint8_t>();
	for (uint8_t i = 0; i < attributesNum; ++i) {
		attributes.emplace_back();
		attributes.back().CreateFromBase(buffer, attributesIdToType);
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

void TextTranslated::SaveToHistory(TextsDatabasePtr db, uint32_t folderId)
{
	auto& buffer = db->GetHistoryBuffer();
	buffer.PushStringWithoutZero<uint8_t>(loginOfLastModifier);
	buffer.Push(timestampCreated);
	buffer.Push(static_cast<uint8_t>(EventCreateText));
	buffer.Push(folderId);
	buffer.PushStringWithoutZero<uint8_t>(id);
}

//===============================================================================
//
//===============================================================================

SerializationBufferPtr TextTranslated::SaveToPacket(uint32_t folderId, const std::string& loginOfModifier) const
{
	auto bufPtr = std::make_shared<SerializationBuffer>();

	bufPtr->PushStringWithoutZero<uint8_t>(loginOfModifier);
	bufPtr->Push(timestampCreated);
	bufPtr->Push(static_cast<uint8_t>(EventCreateText));
	bufPtr->PushStringWithoutZero<uint8_t>(loginOfLastModifier);
	bufPtr->PushStringWithoutZero<uint8_t>(id);

	return bufPtr;
}

//===============================================================================
//
//===============================================================================

void AttributeInText::CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
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
