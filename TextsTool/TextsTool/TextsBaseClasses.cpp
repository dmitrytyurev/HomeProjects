#include <iostream>
#include <ctime>
#include <iostream>
#include <experimental/filesystem>

#include "DbSerializer.h"
#include "../SharedSrc/Shared.h"

void ExitMsg(const std::string& message);


//===============================================================================

TextsDatabase::TextsDatabase(const std::string path, const std::string dbName)
{
	_dbName = dbName;
	_dbSerializer = std::make_unique<DbSerializer>(this);
	_dbSerializer->SetPath(path);
	_dbSerializer->LoadDatabase();
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

void Folder::CreateFromPacket(DeserializationBuffer& buffer, uint32_t ts, uint32_t newId)
{
	id = newId;
	timestampCreated = ts;
	timestampModified = ts;
	parentId = buffer.GetUint32();
	buffer.GetString8(name);
}

//===============================================================================

void Folder::SaveFullDump(SerializationBuffer& buffer) const
{
	buffer.PushUint32(id);
	buffer.PushUint32(timestampCreated);
	buffer.PushUint32(timestampModified);
	buffer.PushString16(name);
	buffer.PushUint32(parentId);
	buffer.PushUint32(texts.size());
	for (const auto& text : texts) {
		text->SaveFullDump(buffer);
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

void TextTranslated::SaveFullDump(SerializationBuffer& buffer) const
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
		attrib.SaveFullDump(buffer);
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
	case AttributeProperty::Translation_t:
	case AttributeProperty::CommonText_t:
	{
		buffer.GetString16(text);
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

void AttributeInText::SaveFullDump(SerializationBuffer& buffer) const
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
