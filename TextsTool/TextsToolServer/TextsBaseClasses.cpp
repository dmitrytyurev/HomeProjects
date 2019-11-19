#include "pch.h"

#include <iostream>
#include <ctime>
#include <iostream>
#include <experimental/filesystem>

#include "Common.h"
#include "DbSerializer.h"
#include "../SharedSrc/Shared.h"

void ExitMsg(const std::string& message);


//===============================================================================

void TextsDatabase::CreateDatabase(const std::string path, const std::string dbName)
{
	_dbName = dbName;
	_dbSerializer = std::make_unique<DbSerializer>(this);
	_dbSerializer->SetPath(path);
	_dbSerializer->SaveDatabase();
}

//===============================================================================

void TextsDatabase::LoadDatabase(const std::string path, const std::string dbName)
{
	_dbName = dbName;
	_dbSerializer = std::make_unique<DbSerializer>(this);
	_dbSerializer->SetPath(path);
	_dbSerializer->LoadDatabaseAndHistory();
}

//===============================================================================

void TextsDatabase::Update(double dt)
{
	_dbSerializer->Update(dt);
}

//===============================================================================

SerializationBuffer& TextsDatabase::GetHistoryBuffer()
{
	return _dbSerializer->GetHistoryBuffer();
}

//===============================================================================

uint32_t TextsDatabase::GetCurrentPosInHistoryFile()
{
	return _dbSerializer->GetCurrentPosInHistoryFile();
}

//===============================================================================

void AttributeProperty::CreateFromBase(DeserializationBuffer& buffer)
{
	id = buffer.GetUint8();
	visiblePosition = buffer.GetUint8();
	isVisible = static_cast<bool>(buffer.GetUint8());
	timestampCreated = buffer.GetUint32();
	buffer.GetString<uint8_t>(name);
	type = buffer.GetUint8();
	param1 = buffer.GetUint32();
	param2 = buffer.GetUint32();
}

//===============================================================================

void AttributeProperty::SaveToBase(SerializationBuffer& buffer) const
{
	buffer.PushUint8(id);
	buffer.PushUint8(visiblePosition);
	uint8_t flag = isVisible;
	buffer.PushUint8(flag);
	buffer.PushUint32(timestampCreated);
	buffer.PushString8(name);
	buffer.PushUint8(type);
	buffer.PushUint32(param1);
	buffer.PushUint32(param2);
}

//===============================================================================

void AttributeProperty::CreateFromHistory(DeserializationBuffer& buffer, uint32_t ts)
{
	id = buffer.GetUint8();
	visiblePosition = buffer.GetUint8();
	isVisible = static_cast<bool>(buffer.GetUint8());
	timestampCreated = ts;
	buffer.GetString<uint8_t>(name);
	type = buffer.GetUint8();
	param1 = buffer.GetUint32();
	param2 = buffer.GetUint32();
}

//===============================================================================

void AttributeProperty::CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId)
{
	id = newId;
	visiblePosition = buffer.GetUint8();
	isVisible = static_cast<bool>(buffer.GetUint8());
	timestampCreated = ts;
	buffer.GetString<uint8_t>(name);
	type = buffer.GetUint8();
	param1 = buffer.GetUint32();
	param2 = 0;
}

//===============================================================================

void AttributeProperty::SaveToHistory(TextsDatabasePtr db, const std::string& loginOfLastModifier) const
{
	auto& buffer = db->GetHistoryBuffer();
	buffer.PushString8(loginOfLastModifier);
	buffer.PushUint32(timestampCreated);
	buffer.PushUint8(EventType::CreateAttribute);
	buffer.PushUint8(id);
	buffer.PushUint8(visiblePosition);
	buffer.PushUint8(isVisible);
	buffer.PushString8(name);
	buffer.PushUint8(type);
	buffer.PushUint32(param1);
	buffer.PushUint32(param2);
}

//===============================================================================

SerializationBufferPtr AttributeProperty::SaveToPacket(const std::string& loginOfModifier) const
{
	auto bufPtr = std::make_shared<SerializationBuffer>();

	bufPtr->PushString8(loginOfModifier);
	bufPtr->PushUint32(timestampCreated);
	bufPtr->PushUint8(EventType::CreateAttribute);
	bufPtr->PushUint8(id);
	bufPtr->PushUint8(visiblePosition);
	bufPtr->PushUint8(isVisible);
	bufPtr->PushString8(name);
	bufPtr->PushUint8(type);
	bufPtr->PushUint32(param1);
	return bufPtr;
}

//===============================================================================

void Folder::CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	id = buffer.GetUint32();
	timestampCreated = buffer.GetUint32();
	timestampModified = buffer.GetUint32();
	buffer.GetString<uint16_t>(name);
	parentId = buffer.GetUint32();
	uint32_t textsNum = buffer.GetUint32();
	for (uint32_t i = 0; i < textsNum; ++i) {
		texts.emplace_back(std::make_shared<TextTranslated>());
		texts.back()->CreateFromBase(buffer, attributesIdToType);
	}
}

//===============================================================================

void Folder::CreateFromHistory(DeserializationBuffer& buffer, uint32_t ts)
{
	timestampCreated = ts;
	timestampModified = ts;
	id = buffer.GetUint32();
	parentId = buffer.GetUint32();
	buffer.GetString<uint8_t>(name);
}

//===============================================================================

void Folder::CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId)
{
	id = newId;
	timestampCreated = ts;
	timestampModified = ts;
	parentId = buffer.GetUint32();
	buffer.GetString<uint8_t>(name);
}

//===============================================================================

void Folder::SaveToBase(SerializationBuffer& buffer) const
{
	buffer.PushUint32(id);
	buffer.PushUint32(timestampCreated);
	buffer.PushUint32(timestampModified);
	buffer.PushString16(name);
	buffer.PushUint32(parentId);
	buffer.PushUint32(texts.size());
	for (const auto& text : texts) {
		text->SaveToBase(buffer);
	}
}

//===============================================================================

void Folder::SaveToHistory(TextsDatabasePtr db, const std::string& loginOfLastModifier) const
{
	auto& buffer = db->GetHistoryBuffer();
	buffer.PushString8(loginOfLastModifier);
	buffer.PushUint32(timestampCreated);
	buffer.PushUint8(EventType::CreateFolder);
	buffer.PushUint32(id);
	buffer.PushUint32(parentId);
	buffer.PushString8(name);
}

//===============================================================================

SerializationBufferPtr Folder::SaveToPacket(const std::string& loginOfModifier) const
{
	auto bufPtr = std::make_shared<SerializationBuffer>();

	bufPtr->PushString8(loginOfModifier);
	bufPtr->PushUint32(timestampCreated);
	bufPtr->PushUint8(EventType::CreateFolder);
	bufPtr->PushUint32(id);
	bufPtr->PushUint32(parentId);
	bufPtr->PushString8(name);
	return bufPtr;
}

//===============================================================================

void TextTranslated::CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	buffer.GetString<uint8_t>(id);
	timestampCreated = buffer.GetUint32();
	timestampModified = buffer.GetUint32();
	buffer.GetString<uint8_t>(loginOfLastModifier);
	offsLastModified = buffer.GetUint32();
	buffer.GetString<uint16_t>(baseText);
	uint8_t attributesNum = buffer.GetUint8();
	for (uint8_t i = 0; i < attributesNum; ++i) {
		attributes.emplace_back();
		attributes.back().CreateFromBase(buffer, attributesIdToType);
	}
}

//===============================================================================

void TextTranslated::SaveToBase(SerializationBuffer& buffer) const
{
	buffer.PushString8(id);
	buffer.PushUint32(timestampCreated);
	buffer.PushUint32(timestampModified);
	buffer.PushString8(loginOfLastModifier);
	buffer.PushUint32(offsLastModified);
	buffer.PushString16(baseText);
	uint8_t attributesNum = static_cast<uint8_t>(attributes.size());
	buffer.PushUint8(attributesNum);
	for (const auto& attrib : attributes) {
		attrib.SaveToBase(buffer);
	}
}

//===============================================================================

void TextTranslated::SaveToHistory(TextsDatabasePtr db, uint32_t folderId)
{
	auto& buffer = db->GetHistoryBuffer();
	buffer.PushString8(loginOfLastModifier);
	buffer.PushUint32(timestampCreated);
	buffer.PushUint8(EventType::CreateText);
	buffer.PushUint32(folderId);
	buffer.PushString8(id);
}

//===============================================================================

SerializationBufferPtr TextTranslated::SaveToPacket(uint32_t folderId, const std::string& loginOfModifier) const
{
	auto bufPtr = std::make_shared<SerializationBuffer>();

	bufPtr->PushString8(loginOfModifier);
	bufPtr->PushUint32(timestampCreated);
	bufPtr->PushUint8(EventType::CreateText);
	bufPtr->PushString8(loginOfLastModifier);
	bufPtr->PushString8(id);

	return bufPtr;
}

//===============================================================================

void AttributeInText::CreateFromBase(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	id = buffer.GetUint8();
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
		flagState = buffer.GetUint8();
		break;

	default:
		ExitMsg("Wrong attribute type!");
	}
}

//===============================================================================

void AttributeInText::SaveToBase(SerializationBuffer& buffer) const
{
	buffer.PushUint8(id);
	switch (type) {
	case AttributeProperty::Translation_t:
	case AttributeProperty::CommonText_t:
	{
		buffer.PushString16(text);
	}
	break;
	case AttributeProperty::Checkbox_t:
		buffer.PushUint8(flagState);
		break;

	default:
		ExitMsg("Wrong attribute type!");
	}
}
