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
#include "MainTableModel.h"
#include "DatabaseManager.h"


//---------------------------------------------------------------

MainTableModel::MainTableModel(TextsDatabasePtr& dataBase) :
	QAbstractTableModel(nullptr), _dataBase(dataBase)
{
}

//---------------------------------------------------------------

int MainTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	if (!_dataBase || !_dataBase->isSynced) {
		return 0;
	}

	return _textsToShow.size();
}

//---------------------------------------------------------------

int MainTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	if (!_dataBase || !_dataBase->isSynced) {
		return 0;
	}

	return _columnsToShow.size();
}

//---------------------------------------------------------------

bool MainTableModel::getTextReferences(const QModelIndex &index, bool needCreateAttrIfNotFound, FoundTextRefs& result)
{
	result.text = _textsToShow[index.row()];

	result.attrInTable = &_dataBase->_attributeProps[_columnsToShow[index.column()]];
	if (result.attrInTable->type == AttributePropertyDataType::Id_t) {
		result.string = &result.text->id;
		return true;
	}
	if (result.attrInTable->type == AttributePropertyDataType::BaseText_t) {
		result.string = &result.text->baseText;
		return true;
	}
	if (result.attrInTable->type == AttributePropertyDataType::CreationTimestamp_t) {
		result.localString = Utils::ConvertTimestampToDate(result.text->timestampCreated);
		result.string = &result.localString;
		return true;
	}
	if (result.attrInTable->type == AttributePropertyDataType::ModificationTimestamp_t) {
		result.localString = result.text->timestampModified == UINT32_MAX ? "Syncing..." : Utils::ConvertTimestampToDate(result.text->timestampModified);
		result.string = &result.localString;
		return true;
	}
	if (result.attrInTable->type == AttributePropertyDataType::LoginOfLastModifier_t) {
		result.string = &result.text->loginOfLastModifier;
		return true;
	}

	for (auto& attribInText: result.text->attributes) {
		if (attribInText.id == result.attrInTable->id) {
			result.string = &attribInText.text;
			result.attrInText = &attribInText;
			return true;
		}
	}
	if (!needCreateAttrIfNotFound) {
		return false;
	}

	result.wasAttrInTextCreated = true;
	result.text->attributes.emplace_back();
	AttributeInText& attribInText = result.text->attributes.back();
	result.attrInText = &attribInText;
	attribInText.id = result.attrInTable->id;
	attribInText.type = result.attrInTable->type;
	result.string = &attribInText.text;

	return true;
}

//---------------------------------------------------------------

int MainTableModel::calcLineByTextId(const std::string& textId)
{
	for (int i=0; i<_textsToShow.size(); ++i) {
		if (_textsToShow[i]->id == textId) {
			return i;
		}
	}
	return -1; // Это нормальная ситуация, если извне изменился текст, который отфильтрован в главной таблице
}

//---------------------------------------------------------------

AttributeProperty* MainTableModel::getAttributeByType(uint8_t attribType)
{
	for (auto& attrib: _dataBase->_attributeProps) {
		if (attrib.type == attribType)  {
			return &attrib;
		}
	}
	return nullptr;
}

//---------------------------------------------------------------

AttributeProperty* MainTableModel::getAttributeById(int attributId)
{
	for (auto& attrib: _dataBase->_attributeProps) {
		if (attrib.id == attributId)  {
			return &attrib;
		}
	}
	return nullptr;
}

//---------------------------------------------------------------

QVariant MainTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || !_dataBase || !_dataBase->isSynced || (role != Qt::DisplayRole &&  role != Qt::EditRole))	{
		return QVariant();
	}

	if(index.column() < 0 ||
			index.column() >= _columnsToShow.size() ||
			index.row() < 0 ||
			index.row() >= _textsToShow.size())	{
		qDebug() << "Warning: " << index.row() << ", " << index.column();
		return QVariant();
	}

	FoundTextRefs textRefs;
	bool isFound = const_cast<MainTableModel*>(this)->getTextReferences(index, false, textRefs);
	if (!isFound) {
		return QVariant();
	}
	return QString(textRefs.string->c_str());
}

//---------------------------------------------------------------

bool MainTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || !_dataBase || !_dataBase->isSynced || role != Qt::EditRole)	{
		return false;
	}

	if(index.column() < 0 ||
			index.column() >= _columnsToShow.size() ||
			index.row() < 0 ||
			index.row() >= _textsToShow.size())	{
		qDebug() << "Warning: " << index.row() << ", " << index.column();
		return false;
	}

	AttributeProperty* attrProp = &_dataBase->_attributeProps[_columnsToShow[index.column()]];
	if (attrProp->type == AttributePropertyDataType::Id_t ||
		attrProp->type == AttributePropertyDataType::CreationTimestamp_t ||
		attrProp->type == AttributePropertyDataType::ModificationTimestamp_t) {
		return false;
	}

	FoundTextRefs textRefs;
	bool isFound = getTextReferences(index, true, textRefs);
	if (!isFound) {
		return false;
	}

	*textRefs.string = value.toString().toUtf8().data();
	DatabaseManager::Instance().OnTextModifiedFromGUI(textRefs);
	std::vector<AttributeProperty*> affectedAttributes;
	OnDataModif(false, TEXTS_RECOLLECT_TYPE::NO, nullptr, false, index.row()); // Не передаём заафеченную колонку, чтобы пересбор текстов и сортировка не происходили до возврата наших изменений с сервера
	return true;
}

//---------------------------------------------------------------

Qt::ItemFlags MainTableModel::flags(const QModelIndex &index) const
{
	if (!index.isValid() || !_dataBase || !_dataBase->isSynced)	{
		return Qt::ItemIsEnabled;
	}

	if(index.column() < 0 ||
			index.column() >= _columnsToShow.size() ||
			index.row() < 0 ||
			index.row() >= _textsToShow.size())	{
		qDebug() << "Warning: " << index.row() << ", " << index.column();
		return Qt::ItemIsEnabled;
	}

	return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

//---------------------------------------------------------------

QHash<int, QByteArray> MainTableModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[0] = "one";
	roles[1] = "two";
	roles[2] = "three";
	return roles;
}

//---------------------------------------------------------------

void MainTableModel::theDataChanged()
{
	//TODO
}

//---------------------------------------------------------------

QVariant MainTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal || section < 0 || section >= _columnsToShow.size())
		return QVariant();

	AttributeProperty& prop = _dataBase->_attributeProps[_columnsToShow[section]];
	return tr(prop.name.c_str());
}

//---------------------------------------------------------------

void MainTableModel::addFolderTextsToShowReq(uint32_t folderId)
{
	for (auto& folder: _dataBase->_folders) {
		if (folder.id == folderId) {
			for (auto& text: folder.texts) {
				_textsToShow.emplace_back(text.get());
			}
			break;
		}
	}
	for (auto& folder: _dataBase->_folders) {
		if (folder.parentId == folderId) {
			addFolderTextsToShowReq(folder.id);
		}
	}
}

//---------------------------------------------------------------

void MainTableModel::SortTextsById()
{
	std::sort(_textsToShow.begin(), _textsToShow.end(), [](TextTranslated* el1, TextTranslated* el2) {
		return el1->id < el2->id;
	});
}

//---------------------------------------------------------------

void MainTableModel::SortTextsByIdBack()
{
	std::sort(_textsToShow.begin(), _textsToShow.end(), [](TextTranslated* el1, TextTranslated* el2) {
		return el2->id < el1->id;
	});
}

//---------------------------------------------------------------

void MainTableModel::SortTextsByCreateTime()
{
	std::sort(_textsToShow.begin(), _textsToShow.end(), [](TextTranslated* el1, TextTranslated* el2) {
		return el1->timestampCreated < el2->timestampCreated;
	});
}

//---------------------------------------------------------------

void MainTableModel::SortTextsByCreateTimeBack()
{
	std::sort(_textsToShow.begin(), _textsToShow.end(), [](TextTranslated* el1, TextTranslated* el2) {
		return el2->timestampCreated < el1->timestampCreated;
	});
}

//---------------------------------------------------------------

void MainTableModel::SortTextsByModifyTime()
{
	std::sort(_textsToShow.begin(), _textsToShow.end(), [](TextTranslated* el1, TextTranslated* el2) {
		return el1->timestampModified < el2->timestampModified;
	});
}

//---------------------------------------------------------------

void MainTableModel::SortTextsByModifyTimeBack()
{
	std::sort(_textsToShow.begin(), _textsToShow.end(), [](TextTranslated* el1, TextTranslated* el2) {
		return el2->timestampModified < el1->timestampModified;
	});
}

//---------------------------------------------------------------

void MainTableModel::SortTextsByLoginOfModifier()
{
	std::sort(_textsToShow.begin(), _textsToShow.end(), [](TextTranslated* el1, TextTranslated* el2) {
		return el1->loginOfLastModifier < el2->loginOfLastModifier;
	});
}

//---------------------------------------------------------------

void MainTableModel::SortTextsByLoginOfModifierBack()
{
	std::sort(_textsToShow.begin(), _textsToShow.end(), [](TextTranslated* el1, TextTranslated* el2) {
		return el2->loginOfLastModifier < el1->loginOfLastModifier;
	});
}


//---------------------------------------------------------------

void MainTableModel::recalcColumnToShowData()
{
	_columnsToShow.clear();
	for (int i=0; i<_dataBase->_attributeProps.size(); ++i) {
		for (int i2=0; i2<_dataBase->_attributeProps.size(); ++i2) {
			auto& attrib = _dataBase->_attributeProps[i2];
			if (attrib.isVisible && attrib.visiblePosition == i) {
				_columnsToShow.push_back(i2);
			}
		}
	}
}

//---------------------------------------------------------------

void MainTableModel::OnDataModif(bool columnsChanged, TEXTS_RECOLLECT_TYPE textsRecollectType,  std::vector<AttributeProperty*>* affectedAttributes, bool sortTypeChanged, int line)
{
	if (columnsChanged) {
		recalcColumnToShowData();
	}

	// Определим нужно ли пересобирать тексты из каталогов
	bool ifNeedRecollectTexts = false;
	if (textsRecollectType != TEXTS_RECOLLECT_TYPE::NO) {
		if (textsRecollectType == TEXTS_RECOLLECT_TYPE::YES) {
			ifNeedRecollectTexts = true;
		}
		else {
			if (affectedAttributes) {
				for (auto attrib: *affectedAttributes) {
					if (attrib->IsFilteredByThisAttribute()) {
						ifNeedRecollectTexts = true;
						break;
					}
				}
			}
		}
	}
	// Если нужно, то пересоберём тексты из каталогов
	if (ifNeedRecollectTexts) {
		_textsToShow.clear();
		for (auto& folder: _dataBase->_folders) {
			if (folder.uiTreeItem->isSelected()) { // Среди всех папок найдём выделенную и запустим от неё рекурсивный обход
				addFolderTextsToShowReq(folder.id);
				break;
			}
		}
	}

	// Определим нужно ли отсортировать тексты
	bool ifNeedSortTexts = false;
	bool isSortOn = MainWindow::Instance().GetSortTypeIndex() != 0;
	if (isSortOn && (sortTypeChanged || ifNeedRecollectTexts)) {
		ifNeedSortTexts = true;
	}
	else {
		if (affectedAttributes) {
			for (auto attrib: *affectedAttributes) {
				if (attrib->IsSortedByThisAttribute()) {
					ifNeedSortTexts = true;
					break;
				}
			}
		}
	}
	// Если нужно, то отсортируем тексты
	if (ifNeedSortTexts) {
		SortTextsByCurrentSortType();
	}

	// Обновим в GUI или все строки или одну заафеченную строку
	if (line == -1 || ifNeedRecollectTexts || ifNeedSortTexts) {
		beginResetModel();
		endResetModel();
	}
	else {
		emit(dataChanged(index(line, 0), index(line, _columnsToShow.size()-1)));
	}
}

void MainTableModel::SortTextsByCurrentSortType()
{
	int sortTypeIndex = MainWindow::Instance().GetSortTypeIndex();
	switch (sortTypeIndex) {
	case 0:
	break;
	case 1:
		SortTextsById();
	break;
	case 2:
		SortTextsByIdBack();
	break;
	case 3:
		SortTextsByCreateTime();
	break;
	case 4:
		SortTextsByCreateTimeBack();
	break;
	case 5:
		SortTextsByModifyTime();
	break;
	case 6:
		SortTextsByModifyTimeBack();
	break;
	case 7:
		SortTextsByLoginOfModifier();
	break;
	case 8:
		SortTextsByLoginOfModifierBack();
	break;
	default:
		Log("SortTextsByCurrentSortTyp: unknown sort type");
	}
}


