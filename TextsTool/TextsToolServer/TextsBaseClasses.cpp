#include <iostream>
#include <ctime>
#include <iostream>
#include <experimental/filesystem>

#include "Common.h"
#include "DbSerializer.h"
#include "../SharedSrc/Shared.h"
#include "Utils.h"

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
	LogDatabase();
}

//===============================================================================

void TextsDatabase::LogDatabase()
{
	Log("--- Database log start -----------------------------------------");
	Log(std::string("  Database: ") + _dbName);
	Log(std::string("  _newAttributeId: ") + std::to_string(_newAttributeId));
	Log("  AttributeProperty's list:");
	for (auto& ap: _attributeProps) {
		ap.Log("    ");
		Log("-----");
	}
	Log(std::string("  _newFolderId: ") + std::to_string(_newFolderId));
	Log("  Folders list:");
	for (auto& folder : _folders) {
		folder.Log("    ");
		Log("  -----");
	}
	Log("--- Database log end -----------------------------------------");
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

void AttributeProperty::LoadFullDump(DeserializationBuffer& buffer)
{
	id = buffer.GetUint8();
	visiblePosition = buffer.GetUint8();
	isVisible = static_cast<bool>(buffer.GetUint8());
	timestampCreated = buffer.GetUint32();
	buffer.GetString8(name);
	type = buffer.GetUint8();
	param1 = buffer.GetUint32();
	param2 = buffer.GetUint32();
}

//===============================================================================

void AttributeProperty::SaveFullDump(SerializationBuffer& buffer) const
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
	buffer.GetString8(name);
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
	buffer.GetString8(name);
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

void AttributeProperty::Log(const std::string& prefix)
{
	::Log(prefix + "id: " + std::to_string(id));
	::Log(prefix + "name: " + name);
	::Log(prefix + "type: " + std::to_string(type));
	::Log(prefix + "visiblePosition: " + std::to_string(visiblePosition));
	::Log(prefix + "isVisible: " + std::to_string(isVisible));
	::Log(prefix + "timestampCreated: " + std::to_string(timestampCreated));
	::Log(prefix + "param1: " + std::to_string(param1));
}


//===============================================================================

void Folder::LoadFullDump(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	id = buffer.GetUint32();
	timestampCreated = buffer.GetUint32();
	timestampModified = buffer.GetUint32();
	buffer.GetString16(name);
	parentId = buffer.GetUint32();
	uint32_t textsNum = buffer.GetUint32();
	for (uint32_t i = 0; i < textsNum; ++i) {
		texts.emplace_back(std::make_shared<TextTranslated>());
		texts.back()->LoadFullDump(buffer, attributesIdToType);
	}
}

//===============================================================================

void Folder::CreateFromHistory(DeserializationBuffer& buffer, uint32_t ts)
{
	timestampCreated = ts;
	timestampModified = ts;
	id = buffer.GetUint32();
	parentId = buffer.GetUint32();
	buffer.GetString8(name);
}

//===============================================================================

void Folder::CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId)
{
	id = newId;
	timestampCreated = ts;
	timestampModified = ts;
	parentId = buffer.GetUint32();
	buffer.GetString8(name);
}

//===============================================================================

void Folder::SaveFullDump(SerializationBuffer& buffer, bool isForSyncMessage) const
{
	buffer.PushUint32(id);
	buffer.PushUint32(timestampCreated);
	buffer.PushUint32(timestampModified);
	buffer.PushString16(name);
	buffer.PushUint32(parentId);
	buffer.PushUint32(texts.size());
	for (const auto& text : texts) {
		text->SaveFullDump(buffer, isForSyncMessage);
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

	bufPtr->PushUint8(EventType::ChangeDataBase);
	bufPtr->PushString8(loginOfModifier);
	bufPtr->PushUint32(timestampCreated);
	bufPtr->PushUint8(EventType::CreateFolder);
	bufPtr->PushUint32(id);
	bufPtr->PushUint32(parentId);
	bufPtr->PushString8(name);
	return bufPtr;
}

//===============================================================================

void Folder::Log(const std::string& prefix)
{
	::Log(prefix + "id: " + std::to_string(id));
	::Log(prefix + "name: " + name);
	::Log(prefix + "parentId: " + std::to_string(parentId));
	::Log(prefix + "timestampCreated: " + std::to_string(timestampCreated));
	::Log(prefix + "timestampModified: " + std::to_string(timestampModified));
	::Log(prefix + "Texts list:");
	for (auto& text : texts) {
		text->Log(prefix + "  ");
		::Log(prefix + "-----");
	}
}

//===============================================================================

void TextTranslated::LoadFullDump(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	buffer.GetString8(id);
	timestampCreated = buffer.GetUint32();
	timestampModified = buffer.GetUint32();
	buffer.GetString8(loginOfLastModifier);
	offsLastModified = buffer.GetUint32();
	buffer.GetString16(baseText);
	uint8_t attributesNum = buffer.GetUint8();
	for (uint8_t i = 0; i < attributesNum; ++i) {
		attributes.emplace_back();
		attributes.back().LoadFullDump(buffer, attributesIdToType);
	}
}

//===============================================================================

void TextTranslated::SaveFullDump(SerializationBuffer& buffer, bool isForSyncMessage) const
{
	buffer.PushString8(id);
	buffer.PushUint32(timestampCreated);
	buffer.PushUint32(timestampModified);
	buffer.PushString8(loginOfLastModifier);
	if (!isForSyncMessage) {
		buffer.PushUint32(offsLastModified);
	}
	buffer.PushString16(baseText);
	uint8_t attributesNum = static_cast<uint8_t>(attributes.size());
	buffer.PushUint8(attributesNum);
	for (const auto& attrib : attributes) {
		attrib.SaveFullDump(buffer);
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

	bufPtr->PushUint8(EventType::ChangeDataBase);
	bufPtr->PushString8(loginOfModifier);
	bufPtr->PushUint32(timestampCreated);
	bufPtr->PushUint8(EventType::CreateText);
	bufPtr->PushUint32(folderId);
	bufPtr->PushString8(id);

	return bufPtr;
}

//===============================================================================

void TextTranslated::Log(const std::string& prefix)
{
	::Log(prefix + "id: " + id);
	::Log(prefix + "timestampCreated: " + std::to_string(timestampCreated));
	::Log(prefix + "timestampModified: " + std::to_string(timestampModified));
	::Log(prefix + "loginOfLastModifier: " + loginOfLastModifier);
	::Log(prefix + "offsLastModified: " + std::to_string(offsLastModified));
	::Log(prefix + "baseText: " + baseText);
	::Log(prefix + "AttributeInText list:");
	for (auto& attr : attributes) {
		attr.Log(prefix + "  ");
		::Log(prefix + "-----");
	}
}


//===============================================================================

void AttributeInText::LoadFullDump(DeserializationBuffer& buffer, const std::vector<uint8_t>& attributesIdToType)
{
	id = buffer.GetUint8();
	if (id >= attributesIdToType.size()) {
		ExitMsg("id >= attributesIdToType.size()");
	}
	type = attributesIdToType[id];
	switch (type) {
	case AttributePropertyType::Translation_t:
	case AttributePropertyType::CommonText_t:
	{
		buffer.GetString16(text);
	}
	break;
	case AttributePropertyType::Checkbox_t:
		flagState = buffer.GetUint8();
		break;

	default:
		ExitMsg("Wrong attribute type!");
	}
}

//===============================================================================

void AttributeInText::SaveFullDump(SerializationBuffer& buffer) const
{
	buffer.PushUint8(id);
	switch (type) {
	case AttributePropertyType::Translation_t:
	case AttributePropertyType::CommonText_t:
	{
		buffer.PushString16(text);
	}
	break;
	case AttributePropertyType::Checkbox_t:
		buffer.PushUint8(flagState);
		break;

	default:
		ExitMsg("Wrong attribute type!");
	}
}

//===============================================================================

void AttributeInText::Log(const std::string& prefix)
{
	::Log(prefix + "id: " + std::to_string(id));
	::Log(prefix + "type: " + std::to_string(type));
	::Log(prefix + "flagState: " + std::to_string(flagState));
	::Log(prefix + "text: " + text);
}

