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

int MainTableModel::calcColumnOfBaseText()
{
	for (int i=0; i<_columnsToShow.size(); ++i) {
		if (_dataBase->_attributeProps[_columnsToShow[i]].type == AttributePropertyDataType::BaseText_t) {
			return i;
		}
	}
	return -1; // Это возможно, если извне изменился текст, в колонке, которая не отображается в таблице
}

//---------------------------------------------------------------

int MainTableModel::calcColumnOfAttributInText(int attributId)
{
	for (int i=0; i<_columnsToShow.size(); ++i) {
		if (_dataBase->_attributeProps[_columnsToShow[i]].id == attributId) {
			return i;
		}
	}
	return -1; // Это возможно, если извне изменился текст, в колонке, которая не отображается в таблице
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

	if(index.column() <= 0 ||
			index.column() >= _columnsToShow.size() ||
			index.row() < 0 ||
			index.row() >= _textsToShow.size())	{
		qDebug() << "Warning: " << index.row() << ", " << index.column();
		return false;
	}

	FoundTextRefs textRefs;
	bool isFound = getTextReferences(index, true, textRefs);
	if (!isFound) {
		return false;
	}

	*textRefs.string = value.toString().toUtf8().data();
	DatabaseManager::Instance().OnTextModifiedFromGUI(textRefs);
	OnDataModif(false, true, false, index.row(), index.column());

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

void MainTableModel::fillRefsToTextsToShow()
{
	_textsToShow.clear();
	for (auto& folder: _dataBase->_folders) {
		if (folder.uiTreeItem->isSelected()) {
			for (auto& text: folder.texts) {
				_textsToShow.emplace_back(text.get());
			}
		}
	}
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

void MainTableModel::OnDataModif(bool selectedFolderChanged, bool oneCellChanged, bool columnsCanChange, int line, int column)
{
	bool canChangeLines = selectedFolderChanged || !oneCellChanged || _isFiltersOn;
	if (canChangeLines) {
		fillRefsToTextsToShow();
	}
	if (columnsCanChange) {
		recalcColumnToShowData();
	}
	if (canChangeLines) {
		beginResetModel();
		endResetModel();
	}
	else {
		if (line != -1 && column != -1) {
			emit(dataChanged(index(line, column), index(line, column)));
		}
	}
}

