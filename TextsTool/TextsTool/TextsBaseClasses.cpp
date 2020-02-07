#include <iostream>
#include <ctime>
#include <iostream>
#include <experimental/filesystem>

#include "Utils.h"
#include "DbSerializer.h"
#include "../SharedSrc/Shared.h"
#include "mainwindow.h"

void ExitMsg(const std::string& message);


//===============================================================================

TextsDatabase::TextsDatabase(const std::string path, const std::string dbName)
{
	_dbName = dbName;
	_dbSerializer = std::make_unique<DbSerializer>(this);
	_dbSerializer->SetPath(path);
	_dbSerializer->LoadDatabase();
//	LogDatabase();
}


//===============================================================================

void TextsDatabase::LogDatabase()
{
	Log("--- Database log start -----------------------------------------");
	Log(std::string("  Database: ") + _dbName);
	Log("  AttributeProperty's list:");
	for (auto& ap: _attributeProps) {
		ap.Log("    ");
		Log("-----");
	}
	Log("  Folders list:");
	for (auto& folder : _folders) {
		folder.Log("    ");
		Log("  -----");
	}
	Log("--- Database log end -----------------------------------------");
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

//if (type == AttributePropertyType::ModificationTimestamp_t) {
//	filterModifyTsMode = LOWER_THAN_TS;  // Режим фильтра по времени модификации текста
//	filterModifyTs = 157830000;            // Относительно какого времени фильтруем
//}

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

bool AttributeProperty::IsFilteredByThisAttribute()
{
	return filterUtf8.length() > 0 ||
		   filterOem.length() > 0 ||
		   filterCreateTsMode != NO_FILTER ||
		   filterModifyTsMode != NO_FILTER;
}

//===============================================================================
bool AttributeProperty::IsSortedByThisAttribute()
{
	auto& selectors = MainWindow::Instance().GetSortSelectors();
	for (auto& selector: selectors) {
		if (selector.second != 255 && type == selector.second) {
			return true;
		}
	}
	return false;
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
	buffer.PushString16(baseText);
	uint8_t attributesNum = static_cast<uint8_t>(attributes.size());
	buffer.PushUint8(attributesNum);
	for (const auto& attrib : attributes) {
		attrib.SaveFullDump(buffer);
	}
}

//===============================================================================

void TextTranslated::Log(const std::string& prefix)
{
	::Log(prefix + "id: " + id);
	::Log(prefix + "timestampCreated: " + std::to_string(timestampCreated));
	::Log(prefix + "timestampModified: " + std::to_string(timestampModified));
	::Log(prefix + "loginOfLastModifier: " + loginOfLastModifier);
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
	case AttributePropertyType::UintValue_t:
	case AttributePropertyType::TranslationStatus_t:
		uintValue = buffer.GetUint8();
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
	case AttributePropertyType::UintValue_t:
	case AttributePropertyType::TranslationStatus_t:
		buffer.PushUint8(uintValue);
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
	::Log(prefix + "uintValue: " + std::to_string(uintValue));
	::Log(prefix + "text: " + text);
}

