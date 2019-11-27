#include <iostream>
#include <ctime>
#include <iostream>
#include <experimental/filesystem>

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
	_dbSerializer->LoadDatabase();
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
