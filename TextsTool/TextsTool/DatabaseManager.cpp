#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <string>
#include <QObject>
#include <QList>
#include <QString>
#include <QDebug>

#include "../SharedSrc/SerializationBuffer.h"
#include "../SharedSrc/DeserializationBuffer.h"
#include "CMessagesRepacker.h"
#include "Utils.h"
#include "DbSerializer.h"
#include "DatabaseManager.h"
#include <QElapsedTimer>


const static std::string databasePath = "D:/Dimka/HomeProjects/TextsTool/DatabaseClient/";
const static int KeyPerTextsNum = 100;  // На такое количество текстов создаётся один ключ для запроса RequestSync

extern QElapsedTimer gTimer;
DatabaseManager* DatabaseManager::pthis = nullptr;


//---------------------------------------------------------------

void DatabaseManager::Init()
{
	if (pthis) {
		ExitMsg("DatabaseManager::Init: already inited");
	}
	pthis = new DatabaseManager;
}

//---------------------------------------------------------------

void DatabaseManager::Deinit()
{
	if (pthis) {
		delete pthis;
		pthis = nullptr;
	}
}

//---------------------------------------------------------------

DatabaseManager& DatabaseManager::Instance()
{
	if (!pthis) {
		ExitMsg("DatabaseManager::Instance: not inited");
	}
	return *pthis;
}

//---------------------------------------------------------------


DatabaseManager::DatabaseManager()
{
	_mainTableModel = std::make_unique<MainTableModel>(_dataBase);
	MainWindow::Instance().SetModelForMainTable(_mainTableModel.get());
}

//---------------------------------------------------------------

void DatabaseManager::SaveDatabase()
{
	if (!_dataBase || !_dataBase->isSynced) {
		return;
	}
	_dataBase->_dbSerializer->SaveDatabase();
}

//---------------------------------------------------------------

void DatabaseManager::LoadBaseAndRequestSync(const std::string& dbName)
{
	_dataBase = std::make_shared<TextsDatabase>(databasePath, dbName);

	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	auto& buf = *_msgsQueueOut.back();
	buf.PushUint8(EventType::RequestSync);
	buf.PushString8(_dataBase->_dbName);
	buf.PushUint32(_dataBase->_folders.size());
	for (auto& folder: _dataBase->_folders) {
		buf.PushUint32(folder.id);
		buf.PushUint32(folder.timestampModified);

		if (folder.texts.size() < KeyPerTextsNum) {
			buf.PushUint32(0);
			continue;
		}

		// Заполняем ключи текстов и ссылки на них для быстрой сортировки по ключам

		std::vector<std::vector<uint8_t>> textsKeys;       // Ключи серверных текстов текущей папки (для сортировки и разбиения на интевалы)

		textsKeys.resize(folder.texts.size());
		_textsKeysRefs.resize(folder.texts.size());

		for (int i=0; i<folder.texts.size(); ++i) {
			_textsKeysRefs[i] = i;
			MakeKey(folder.texts[i]->timestampModified, folder.texts[i]->id, textsKeys[i]); // Склеивает ts модификации текста и текстовый айдишник текста в "ключ"
		}

		// Сортируем ссылки на заполненные ключи текстов
		std::sort(_textsKeysRefs.begin(), _textsKeysRefs.end(), [&textsKeys](int el1, int el2) {
				auto& el1Ref = textsKeys[el1];
				auto& el2Ref = textsKeys[el2];
				return IfKeyALess(&el1Ref[0], el1Ref.size(), &el2Ref[0], el2Ref.size());
			});

		int selectedKeysNum = folder.texts.size() / KeyPerTextsNum;
		buf.PushUint32(selectedKeysNum);
		_intervals.resize(selectedKeysNum + 1);
		int offs = (folder.texts.size() - (selectedKeysNum - 1) * KeyPerTextsNum) / 2;
		// Добавляем в буфер ключи текстов и заполняем границы интервалов
		int v = 0;
		for (int i=0; i<selectedKeysNum; ++i) {
			int index = i*KeyPerTextsNum + offs;
			buf.PushVector8(textsKeys[_textsKeysRefs[index]]);
			_intervals[i].firstTextIdx = v;
			_intervals[i].textsNum = index - v;
			v = index;
		}
		_intervals[selectedKeysNum].firstTextIdx = v;
		_intervals[selectedKeysNum].textsNum = folder.texts.size() - v;

		// Расчёт и добавляем в буфер CRC64 интервалов
		for (auto& interval: _intervals) {
			uint64_t crc = 0;
			for (uint32_t i = interval.firstTextIdx; i < (int)interval.firstTextIdx + interval.textsNum; ++i) {
				crc = Utils::AddHash(crc, textsKeys[_textsKeysRefs[i]], i == interval.firstTextIdx);
			}
			buf.PushUint64(crc);
		}
	}
}

//---------------------------------------------------------------

void DatabaseManager::ApplyDiffForSync(DeserializationBuffer& buf)
{
	std::string dbName;
	buf.GetString8(dbName);
	if (_dataBase && _dataBase->_dbName != dbName) {
		_dataBase->_dbSerializer->SaveDatabase();
		_dataBase.reset();
	}
	if (!_dataBase) {
		_dataBase = std::make_shared<TextsDatabase>(databasePath, dbName);
	}

	// Загружаем атрибуты заново с сервера
	_dataBase->_attributeProps.clear();
	int attribsNum = buf.GetUint32();
	for (int i=0; i<attribsNum; ++i) {
		_dataBase->_attributeProps.emplace_back();
		_dataBase->_attributeProps.back().LoadFullDump(buf);
	}

	// Удаляем каталоги, которых уже нет на сервере
	int foldersToDel = buf.GetUint32();
	for (int i=0; i<foldersToDel; ++i) {
		int folderToDelId = buf.GetUint32();
		auto& folders = _dataBase->_folders;
		folders.erase(std::remove_if(folders.begin(), folders.end(), [folderToDelId](const auto& el) { return el.id == folderToDelId; }), folders.end());
	}

	// Заполним таблицу для быстрой перекодировки id атрибута в его type
	std::vector<uint8_t> attributesIdToType;
	for (auto& attrib: _dataBase->_attributeProps) {
		if (attrib.id >= attributesIdToType.size()) {
			attributesIdToType.resize(attrib.id + 1);
		}
		attributesIdToType[attrib.id] = attrib.type;
	}

	// Добавляем каталоги с текстами, которых нет на клиенте
	int foldersToCreate = buf.GetUint32();
	for (int i=0; i<foldersToCreate; ++i) {
		_dataBase->_folders.emplace_back();
		_dataBase->_folders.back().LoadFullDump(buf, attributesIdToType);
	}

	// Патчим каталоги с текстами, которые есть на клиенте, но устаревшие
	int foldersToPatch = buf.GetUint32();
	for (int i=0; i<foldersToPatch; ++i) {
		int folderId = buf.GetUint32();
		auto& f = _dataBase->_folders;
		auto folderIt = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
		if (folderIt == std::end(f)) {
			ExitMsg("folderFound == std::end(f)");
		}
		buf.GetString16(folderIt->name);
		folderIt->parentId = buf.GetUint32();
		folderIt->timestampModified = buf.GetUint32();
		uint8_t nextDataType = buf.GetUint8();
		if (nextDataType == FolderDataTypeForSyncMsg::AllTexts) {
			// Присланы все тексты каталога
			folderIt->texts.clear();
			int textsNum = buf.GetUint32();
			for (int i2 = 0; i2 < textsNum; ++i2) {
				folderIt->texts.emplace_back(std::make_shared<TextTranslated>());
				folderIt->texts.back()->LoadFullDump(buf, attributesIdToType);
			}
		}
		else {
			// Прислана часть текстов каталога (только для изменившихся интервалов)
			if (nextDataType != FolderDataTypeForSyncMsg::DiffTexts) {
				ExitMsg("nextDataType != FolderDataTypeForSyncMsg::DiffTexts");
			}
			int changedIntervalsNum = buf.GetUint32();
			for (int i2 = 0; i2 < changedIntervalsNum; ++i2) {
				int changedIntervalId = buf.GetUint32();
				// Обнулим указатели на тексты данного интервала (указатели из вектора удалим после обработки всех интервалов)
				int textsInIntervalOldNum = _intervals[changedIntervalId].textsNum;
				for (int i3 = 0; i3 < textsInIntervalOldNum; ++i3) {
					int index = i3 + _intervals[changedIntervalId].firstTextIdx;
					folderIt->texts[_textsKeysRefs[index]].reset();
				}
				// Добавим новые тексты интервала в папку
				int textsInIntervalNewNum = buf.GetUint32();
				for (int i3 = 0; i3 < textsInIntervalNewNum; ++i3) {
					folderIt->texts.emplace_back(std::make_shared<TextTranslated>());
					folderIt->texts.back()->LoadFullDump(buf, attributesIdToType);
				}
			}
			// Удалим из вектора указатели на тексты, которые были обнулены выше
			folderIt->texts.erase(std::remove_if(folderIt->texts.begin(), folderIt->texts.end(), [](const auto& el) { return el; }), folderIt->texts.end());
		}
	}

	_dataBase->isSynced = true;
}

//---------------------------------------------------------------

void DatabaseManager::Update(Ui::MainWindow* ui)
{
	_mainTableModel->Update(ui);
	Repacker::RepackPacketsInToMessages(CHttpManager::Instance(), _msgsQueueIn);
	Repacker::RepackMessagesOutToPackets(_msgsQueueOut, CHttpManager::Instance());

	for (auto& msg: _msgsQueueIn) {
		Log("Message:  ");
		Utils::LogBuf(msg->_buffer);
		ProcessMessageFromServer(msg->_buffer);
	}
	_msgsQueueIn.resize(0);
}

//---------------------------------------------------------------

void DatabaseManager::TreeSelectionChanged()
{
	_mainTableModel->OnDataModif(false, TEXTS_RECOLLECT_TYPE::YES, nullptr, false, -1);
}

//---------------------------------------------------------------

void DatabaseManager::SortSelectionChanged(int index)
{
	_mainTableModel->OnDataModif(false, TEXTS_RECOLLECT_TYPE::NO, nullptr, true, -1);
}

//---------------------------------------------------------------

void DatabaseManager::ProcessMessageFromServer(const std::vector<uint8_t>& buf)
{
	DeserializationBuffer dbuf(buf); // !!! Неоптимально! Этот буфер может быть сотню Мб! Подумать о работе по указателю без копирования данных
	uint8_t msgType = dbuf.GetUint8();

	switch(msgType) {
	case EventType::ReplySync:
	{
		Log("Msg: ReplySync");
		ApplyDiffForSync(dbuf);
		_dataBase->_dbSerializer->SaveDatabase();
//_dataBase->LogDatabase();
		AdjustFolderView(UINT32_MAX, nullptr);
		MainWindow::Instance().getTreeWidget()->expandAll();
		_mainTableModel->OnDataModif(true, TEXTS_RECOLLECT_TYPE::YES, nullptr, false, -1);
	}
	break;
	case EventType::ChangeDataBase:
	{
		Log("Msg: ChangeDataBase");

		std::string loginOfModifier;
		dbuf.GetString8(loginOfModifier);
		uint32_t ts = dbuf.GetUint32();
		uint8_t actionType = dbuf.GetUint8();
		switch (actionType) {
		case EventType::ChangeBaseText:
		{
			Log("Msg: ChangeBaseText");
			std::vector<AttributeProperty*> affectedAttributes;
			std::string textId = ModifyDbChangeBaseText(dbuf, ts, loginOfModifier, &affectedAttributes);
			_mainTableModel->OnDataModif(false, TEXTS_RECOLLECT_TYPE::IF_COLUMNS_AFFECTED, &affectedAttributes, false, _mainTableModel->calcLineByTextId(textId));
		}
		break;
		case EventType::ChangeAttributeInText:
		{
			Log("Msg: ChangeAttributeInText");
			std::vector<AttributeProperty*> affectedAttributes;
			std::string textId = ModifyDbChangeAttributeInText(dbuf, ts, loginOfModifier, &affectedAttributes);
			_mainTableModel->OnDataModif(false, TEXTS_RECOLLECT_TYPE::IF_COLUMNS_AFFECTED, &affectedAttributes, false, _mainTableModel->calcLineByTextId(textId));
		}
		break;
		case EventType::CreateText:
		{
			Log("Msg: ChangeCreateText");
			std::string textId = ModifyDbCreateText(dbuf, ts, loginOfModifier);
			_mainTableModel->OnDataModif(false, TEXTS_RECOLLECT_TYPE::YES, nullptr, false, -1);
			if (loginOfModifier == CHttpManager::Instance().GetLogin()) {  // Если с сервера вернулось наше сообщение о создании текста, то перемотаем таблицу на него и включим редактирование BaseText
				int line = _mainTableModel->calcLineByTextId(textId);
				int column = _mainTableModel->calcColumnByType(AttributePropertyType::BaseText_t);
				if (line != -1 && column != -1) {
					MainWindow::Instance().SetFocusToTableCellAndStartEdit(_mainTableModel->index(line, column));
				}
			}
		}
		break;
		default:
			Log("Unknown actionType: " + std::to_string(actionType));
		}
	}
	break;
	default:
		Log("Unknown msgType:" + std::to_string(msgType));
	}
}

//---------------------------------------------------------------

// Важно: при любых изменениях текстов со стороны GUI надо записать -1 во timestamp’ы текста и непосредственный каталог

void DatabaseManager::OnTextModifiedFromGUI(const FoundTextRefs& textRefs)
{
	if (textRefs.attrInTable->type == AttributePropertyType::BaseText_t) {
		SendMsgChangeBaseText(textRefs);
	}
	else {
		SendMsgChangeTextAttrib(textRefs);
	}
	ResetTextAndFolderTimestamps(textRefs);
}

//---------------------------------------------------------------

void DatabaseManager::ResetTextAndFolderTimestamps(const FoundTextRefs& textRefs)
{
	textRefs.text->timestampModified = UINT32_MAX; // Это признак, что были изменения из GUI. Чтобы при стартовой синхронизации с сервером, точно была разница и эти данные пришли с сервера
	for (auto& folder: _dataBase->_folders) {
		for (auto& text: folder.texts) {
			if (text->id == textRefs.text->id) {
				folder.timestampModified = UINT32_MAX; 	// Это признак, что были изменения из GUI. Чтобы при стартовой синхронизации с сервером, точно была разница и эти данные пришли с сервера
				return;
			}
		}
	}
	ExitMsg("ResetTextAndFolderTimestamps: text not found in folder");
}

//---------------------------------------------------------------

void DatabaseManager::SendMsgChangeTextAttrib(const FoundTextRefs& textRefs)
{
	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	auto& buf = *_msgsQueueOut.back();
	buf.PushUint8(EventType::ChangeAttributeInText);
	buf.PushString8(textRefs.text->id);
	buf.PushUint8(textRefs.attrInText->id);
	buf.PushUint8(textRefs.attrInText->type);
	buf.PushString16(textRefs.attrInText->text);
}

//---------------------------------------------------------------

void DatabaseManager::SendMsgChangeBaseText(const FoundTextRefs& textRefs)
{
	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	auto& buf = *_msgsQueueOut.back();
	buf.PushUint8(EventType::ChangeBaseText);
	buf.PushString8(textRefs.text->id);
	buf.PushString16(textRefs.text->baseText);
}

//---------------------------------------------------------------

std::string DatabaseManager::ModifyDbChangeBaseText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifier, std::vector<AttributeProperty*>* affectedAttributes)
{
	std::string textId;
	dbuf.GetString8(textId);
	std::string newBaseText;
	dbuf.GetString16(newBaseText);

	TextTranslatedPtr tmpTextPtr;
	for (auto& f : _dataBase->_folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslatedPtr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			tmpTextPtr = *result;
			f.timestampModified = ts;
			break;
		}
	}

	if (!tmpTextPtr) {
		return "";
	}

	// Нашли текст, модифицируем его
	tmpTextPtr->baseText = newBaseText;
	tmpTextPtr->loginOfLastModifier = loginOfModifier;
	tmpTextPtr->timestampModified = ts;

	// Заполняем атрибуты таблицы (колонки), которые у данного текста мы модифицировали
	AttributeProperty* affectedAttrib = _mainTableModel->getAttributeByType(AttributePropertyType::BaseText_t);
	if (affectedAttrib) {
		affectedAttributes->emplace_back(affectedAttrib);
	}
	affectedAttrib = _mainTableModel->getAttributeByType(AttributePropertyType::ModificationTimestamp_t);
	if (affectedAttrib) {
		affectedAttributes->emplace_back(affectedAttrib);
	}
	affectedAttrib = _mainTableModel->getAttributeByType(AttributePropertyType::LoginOfLastModifier_t);
	if (affectedAttrib) {
		affectedAttributes->emplace_back(affectedAttrib);
	}

	return textId;
}

//---------------------------------------------------------------

std::string DatabaseManager::ModifyDbChangeAttributeInText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifier, std::vector<AttributeProperty*>* affectedAttributes)
{
	std::string textId;
	dbuf.GetString8(textId);
	uint8_t attribId = dbuf.GetUint8();
	uint8_t attribDataType = dbuf.GetUint8();
	std::string text;
	uint8_t flagState = 0;
	switch(attribDataType) {
	case AttributePropertyType::Translation_t:
	case AttributePropertyType::CommonText_t:
		dbuf.GetString16(text);
	break;
	case AttributePropertyType::Checkbox_t:
		flagState = dbuf.GetUint8();
	break;
	default:
		ExitMsg("DatabaseManager::ModifyDbChangeAttributeInText: unknown dataType");
	break;
	}

	TextTranslatedPtr tmpTextPtr;
	for (auto& f : _dataBase->_folders) {
		auto result = std::find_if(std::begin(f.texts), std::end(f.texts), [&textId](const TextTranslatedPtr& el) { return el->id == textId; });
		if (result != std::end(f.texts)) {
			tmpTextPtr = *result;
			f.timestampModified = ts;
			break;
		}
	}

	if (!tmpTextPtr) {
		Log("DatabaseManager::ModifyDbChangeAttributeInText: textId not found");
		return "";
	}

	// Нашли текст, ищем атрибут в нём и модифицируем его
	tmpTextPtr->loginOfLastModifier = loginOfModifier;
	tmpTextPtr->timestampModified = ts;

	AttributeInText* attribInTextToModify = nullptr;
	for (auto& attribInText: tmpTextPtr->attributes) {
		if (attribInText.id	== attribId) {
			attribInTextToModify = &attribInText;
		}
	}
	if (!attribInTextToModify) {
		tmpTextPtr->attributes.emplace_back();
		attribInTextToModify = &tmpTextPtr->attributes.back();
	}

	switch(attribDataType) {
	case AttributePropertyType::Translation_t:
	case AttributePropertyType::CommonText_t:
		attribInTextToModify->text = text;
		if (text.empty()) {
			int indexElement = attribInTextToModify - &tmpTextPtr->attributes[0];
			tmpTextPtr->attributes.erase(tmpTextPtr->attributes.begin() + indexElement);
		}
	break;
	case AttributePropertyType::Checkbox_t:
		attribInTextToModify->flagState = flagState;
	break;
	}

	// Заполняем атрибуты таблицы (колонки), которые у данного текста мы модифицировали
	AttributeProperty* affectedAttrib = _mainTableModel->getAttributeById(attribId);
	if (affectedAttrib) {
		affectedAttributes->emplace_back(affectedAttrib);
	}
	affectedAttrib = _mainTableModel->getAttributeByType(AttributePropertyType::ModificationTimestamp_t);
	if (affectedAttrib) {
		affectedAttributes->emplace_back(affectedAttrib);
	}
	affectedAttrib = _mainTableModel->getAttributeByType(AttributePropertyType::LoginOfLastModifier_t);
	if (affectedAttrib) {
		affectedAttributes->emplace_back(affectedAttrib);
	}

	return textId;
}

//---------------------------------------------------------------

std::string DatabaseManager::ModifyDbCreateText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifie)
{
	uint32_t folderId = dbuf.GetUint32();
	std::string textId;
	dbuf.GetString8(textId);

	auto& f = _dataBase->_folders;
	auto result = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
	if (result != std::end(f)) {
		result->texts.emplace_back(new TextTranslated);
		TextTranslated* text = result->texts.back().get();
		text->id = textId;
		text->timestampCreated = ts;
		text->timestampModified = ts;
		text->loginOfLastModifier = loginOfModifie;
	}
	return textId;
}

//---------------------------------------------------------------

void DatabaseManager::AdjustFolderView(uint32_t parentId, QTreeWidgetItem *parentTreeItem)
{
	for (auto& folder: _dataBase->_folders) {
		if (folder.parentId == parentId) {
			if (parentId == UINT32_MAX) {  // Корневая папка
				QTreeWidgetItem* treeItem = new QTreeWidgetItem(MainWindow::Instance().getTreeWidget());
				folder.uiTreeItem = treeItem;
				treeItem->setText(0, QString::fromStdString(folder.name));
				AdjustFolderView(folder.id, treeItem);
			}
			else { // Некорневая папка
				QTreeWidgetItem *treeItem = new QTreeWidgetItem();
				folder.uiTreeItem = treeItem;
				treeItem->setText(0, QString::fromStdString(folder.name));
				parentTreeItem->addChild(treeItem);
			}
		}
	}
}

//---------------------------------------------------------------

void DatabaseManager::SendMsgCreateNewText(const std::string& textId)
{
	// Найти выделенный каталог (в него будем добавлять текст)
	uint32_t folderId = UINT32_MAX;
	for (auto& folder: _dataBase->_folders) {
		if (folder.uiTreeItem->isSelected()) {
			folderId = folder.id;
			break;
		}
	}
	if (folderId == UINT32_MAX) {
		return;
	}
	// Послать сообщение о создании текста
	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	auto& buf = *_msgsQueueOut.back();
	buf.PushUint8(EventType::CreateText);
	buf.PushUint32(folderId);
	buf.PushString8(textId);
}
