#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <string>
#include <QObject>
#include <QList>
#include <QString>
#include <QDebug>
#include <QLineEdit>

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

AttributeInText* getAttributeInText(TextTranslated* text, uint8_t columnId)
{
	for (auto& attribInText : text->attributes) {
		if (attribInText.id == columnId) {
			return &attribInText;
		}
	}
	return nullptr;
}

//---------------------------------------------------------------

bool isTranslateEmpty(TextTranslated* text, uint8_t columnId)
{
	AttributeInText* attribInText = getAttributeInText(text, columnId);
	if (!attribInText) {
		return true;
	}
	return attribInText->text.empty();
}

//---------------------------------------------------------------

bool MainTableModel::getTextReferences(const QModelIndex &index, bool needCreateAttrIfNotFound, FoundTextRefs& result)
{
	result.text = _textsToShow[index.row()];

	result.attrInTable = &_dataBase->_attributeProps[_columnsToShow[index.column()].attribIndex];
	if (result.attrInTable->type == AttributePropertyType::Id_t) {
		result.stringToShowInCell = result.text->id;
		return true;
	}
	if (result.attrInTable->type == AttributePropertyType::BaseText_t) {
		result.stringToShowInCell = result.text->baseText;
		return true;
	}
	if (result.attrInTable->type == AttributePropertyType::CreationTimestamp_t) {
		result.stringToShowInCell = Utils::ConvertTimestampToDate(result.text->timestampCreated);
		return true;
	}
	if (result.attrInTable->type == AttributePropertyType::ModificationTimestamp_t) {
		result.stringToShowInCell = result.text->timestampModified == UINT32_MAX ? "Syncing..." : Utils::ConvertTimestampToDate(result.text->timestampModified);
		return true;
	}
	if (result.attrInTable->type == AttributePropertyType::LoginOfLastModifier_t) {
		result.stringToShowInCell = result.text->loginOfLastModifier;
		return true;
	}
	if (result.attrInTable->type == AttributePropertyType::TranslationStatus_t) {
		AttributeInText* attributeInText = getAttributeInText(result.text, result.attrInTable->id);
		result.attrInText = attributeInText; // AttributeInText с информацией о данной колонке
		if (!attributeInText && !isTranslateEmpty(result.text, result.attrInTable->param2)) { // (см. описание структуры что в param2
			result.stringToShowInCell = "+";
		}
		else {
			result.stringToShowInCell = "-";
		}
		return true;
	}
	if (result.attrInTable->type == AttributePropertyType::Translation_t || result.attrInTable->type == AttributePropertyType::CommonText_t) {
		AttributeInText* attributeInText = getAttributeInText(result.text, result.attrInTable->id);
		if (!attributeInText) { // Не нашли AttributeInText с информацией о данной колонке
			if (!needCreateAttrIfNotFound) {
				return false;
			}
			result.wasAttrInTextCreated = true;
			result.text->attributes.emplace_back();
			AttributeInText& attribInText = result.text->attributes.back();
			result.attrInText = &attribInText;
			attribInText.id = result.attrInTable->id;
			attribInText.type = result.attrInTable->type;
			result.stringToShowInCell = attribInText.text;
			return true;
		}
		result.attrInText = attributeInText; // Нашли AttributeInText с информацией о данной колонке
		result.stringToShowInCell = attributeInText->text;
		return true;
	}
	if (result.attrInTable->type == AttributePropertyType::UintValue_t) {
		AttributeInText* attributeInText = getAttributeInText(result.text, result.attrInTable->id);
		if (!attributeInText) { // Не нашли AttributeInText с информацией о данной колонке
			if (!needCreateAttrIfNotFound) {
				return false;
			}
			result.wasAttrInTextCreated = true;
			result.text->attributes.emplace_back();
			AttributeInText& attribInText = result.text->attributes.back();
			result.attrInText = &attribInText;
			attribInText.id = result.attrInTable->id;
			attribInText.type = result.attrInTable->type;
			return true;
		}
		result.attrInText = attributeInText; // Нашли AttributeInText с информацией о данной колонке
		result.stringToShowInCell = std::to_string(attributeInText->uintValue);
		return true;
	}
	return false;
}

//---------------------------------------------------------------


void MainTableModel::ApplyEnteredTextIntoCell(FoundTextRefs& result, std::string textEntered)
{
	if (result.attrInTable->type == AttributePropertyType::BaseText_t) {
		result.text->baseText = textEntered;
		return;
	}
	if (result.attrInTable->type == AttributePropertyType::Translation_t || result.attrInTable->type == AttributePropertyType::CommonText_t) {
		if (result.attrInText) {
			result.attrInText->text = textEntered;
		}
	}
	if (result.attrInTable->type == AttributePropertyType::UintValue_t) {
		if (result.attrInText) {
			if (textEntered.empty()) {
				result.attrInText->uintValue = UINT_NO_VALUE;
			}
			else {
				result.attrInText->uintValue = std::stoi(textEntered);
			}
		}
		return;
	}
	if (result.attrInTable->type == AttributePropertyType::TranslationStatus_t) {
		if (textEntered == "-") {
			if (!result.attrInText) {
				AttributeInText attribInText;
				attribInText.uintValue = 1;
				attribInText.id = result.attrInTable->id;
				attribInText.type = result.attrInTable->type;
				result.text->attributes.push_back(attribInText);
				result.attrInText = &result.text->attributes.back();
			}
		}
		else if (textEntered == "+") {  // Если текст перевода не пустой, то удалить статус-атрибут из текста (это будет означать, что текст перевода синхронизирован)
			if (result.attrInText) {   
				auto& attrs = result.text->attributes;
				uint32_t translatedTextColumnId = result.attrInTable->param2; // id колонки, в которой переведённый текст, статус которого мы редактируем
				auto TranslatedTextIter = std::find_if(std::begin(attrs), std::end(attrs), [translatedTextColumnId](const AttributeInText& el) { return el.id == translatedTextColumnId; });
				if (TranslatedTextIter != std::end(attrs) && !TranslatedTextIter->text.empty()) {
					result.attrInText->uintValue = UINT_NO_VALUE;
				}
			}
		}
		return;
	}
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

int MainTableModel::calcColumnByType(uint8_t attribType)
{
	for (int i = 0; i < _columnsToShow.size(); ++i) {
		if (_dataBase->_attributeProps[_columnsToShow[i].attribIndex].type == attribType) {
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
	if (!index.isValid() || !_dataBase || !_dataBase->isSynced)	{
		return QVariant();
	}

	if(index.column() < 0 ||
			index.column() >= _columnsToShow.size() ||
			index.row() < 0 ||
			index.row() >= _textsToShow.size())	{
		qDebug() << "Warning: " << index.row() << ", " << index.column();
		return QVariant();
	}

	if (role == Qt::BackgroundRole) {
		if (index.column() == 2 && index.row() == 1) {
			return QColor(0, 255, 0);
		}
		return QVariant();
	}

	if (role != Qt::DisplayRole &&  role != Qt::EditRole) {
		return QVariant();
	}

	FoundTextRefs textRefs;
	bool isFound = const_cast<MainTableModel*>(this)->getTextReferences(index, false, textRefs);
	if (!isFound) {
		return QVariant();
	}
	return QString(textRefs.stringToShowInCell.c_str());
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

	AttributeProperty* attrProp = &_dataBase->_attributeProps[_columnsToShow[index.column()].attribIndex];
	if (attrProp->type == AttributePropertyType::Id_t ||
		attrProp->type == AttributePropertyType::CreationTimestamp_t ||
		attrProp->type == AttributePropertyType::ModificationTimestamp_t) {
		return false;
	}

	FoundTextRefs textRefs;
	bool isFound = getTextReferences(index, true, textRefs);
	if (!isFound) {
		return false;
	}

	ApplyEnteredTextIntoCell(textRefs, value.toString().toUtf8().data());
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

	AttributeProperty& prop = _dataBase->_attributeProps[_columnsToShow[section].attribIndex];
	return tr(prop.name.c_str());
}

//---------------------------------------------------------------

bool MainTableModel::ifTextPassesFilters(TextTranslatedPtr& text, std::vector<AttributeProperty*>& attribsFilter)
{
	for (auto attrib: attribsFilter) {
		switch (attrib->type) {
			case AttributePropertyType::LoginOfLastModifier_t:
			{
				if (text->loginOfLastModifier.find(attrib->filterOem) == std::string::npos) {
					return false;
				}
			}
			break;
			case AttributePropertyType::Id_t:
			{
				if (text->id.find(attrib->filterOem) == std::string::npos) {
					return false;
				}
			}
			break;
			case AttributePropertyType::CreationTimestamp_t:
			{
				if (attrib->filterCreateTsMode == AttributeProperty::GREATER_THAN_TS) {
					if (text->timestampCreated < attrib->filterCreateTs) {
						return false;
					}
				}
				else {
					if (text->timestampCreated > attrib->filterCreateTs) {
						return false;
					}
				}
			}
			break;
			case AttributePropertyType::ModificationTimestamp_t:
			{
				if (attrib->filterModifyTsMode == AttributeProperty::GREATER_THAN_TS) {
					if (text->timestampModified < attrib->filterModifyTs) {
						return false;
					}
				}
				else {
					if (text->timestampModified > attrib->filterModifyTs) {
						return false;
					}
				}
			}
			break;
			case AttributePropertyType::BaseText_t:
			{
				QString qstr = QString::fromStdString(text->baseText);
				if (!qstr.contains(attrib->filterUtf8)) {
					return false;
				}
			}
			break;
			case AttributePropertyType::Translation_t:
			case AttributePropertyType::CommonText_t:
			{
				bool found = false;
				for (auto& attribInText: text->attributes) {
					if (attribInText.id != attrib->id) {
						continue;
					}
					found = true;
					QString qstr = QString::fromStdString(attribInText.text);
					if (!qstr.contains(attrib->filterUtf8)) {
						return false;
					}
				}
				if (!found) {
					return false;
				}
			}
			break;
		}
	}
	return true;
}

//---------------------------------------------------------------

void MainTableModel::addFolderTextsToShowReq(uint32_t folderId, std::vector<AttributeProperty*>& attribsFilter)
{
	for (auto& folder: _dataBase->_folders) {
		if (folder.id == folderId) {
			for (auto& text: folder.texts) {
				if (ifTextPassesFilters(text, attribsFilter)) {
					_textsToShow.emplace_back(text.get());
				}
			}
			break;
		}
	}
	for (auto& folder: _dataBase->_folders) {
		if (folder.parentId == folderId) {
			addFolderTextsToShowReq(folder.id, attribsFilter);
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
				COLUMN_INFO ci;
				ci.attribIndex = i2;
				ci.lineEdit = std::make_unique<QLineEdit>(&MainWindow::Instance());
				connect(ci.lineEdit.get(), SIGNAL(editingFinished()), this, SLOT(filterEditFinished()) );
				ci.lineEdit->show();
				_columnsToShow.push_back(std::move(ci));
			}
		}
	}
}

//---------------------------------------------------------------

void MainTableModel::filterEditFinished()
{
	int attributIndex = -1;
	QObject* obj = QObject::sender();
	for (auto& column : _columnsToShow) {
		if (column.lineEdit.get() == obj) {
			attributIndex = column.attribIndex;
			break;
		}
	}

	for (auto& attrib: _dataBase->_attributeProps) {
		if (attrib.id == attributIndex) {
			if (attrib.type == AttributePropertyType::Id_t || attrib.type == AttributePropertyType::LoginOfLastModifier_t) {
				attrib.filterOem = static_cast<QLineEdit*>(obj)->text().toLocal8Bit().constData();
			}
			else if (attrib.type == AttributePropertyType::BaseText_t ||
					 attrib.type == AttributePropertyType::CommonText_t ||
					 attrib.type == AttributePropertyType::Translation_t)
			{
				attrib.filterUtf8 = static_cast<QLineEdit*>(obj)->text();
			}
			OnDataModif(false, TEXTS_RECOLLECT_TYPE::YES, nullptr, false, -1);
			break;
		}
	}
}


//---------------------------------------------------------------

void MainTableModel::recollectTextsFromSelectedFolder()
{
	std::vector<AttributeProperty*> attribsFilter;
	// Выберем колонки, по которым задан фильтр
	for (auto& attrib: _dataBase->_attributeProps) {
		if (attrib.IsFilteredByThisAttribute()) {
			attribsFilter.emplace_back(&attrib);
		}
	}

	// Заполним _textsToShow текстами, начиная с выбранного в дереве каталога рекурсивно (с учётом фильтра)
	_textsToShow.clear();
	uint32_t folderId = DatabaseManager::Instance().GetSelectedFolder();
	if (folderId == UINT32_MAX) {
		return;
	}
	addFolderTextsToShowReq(folderId, attribsFilter);
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
		recollectTextsFromSelectedFolder();
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

//---------------------------------------------------------------

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

//---------------------------------------------------------------

void MainTableModel::Update(Ui::MainWindow* ui)
{
	// Обновить границы QLineEdit'ов хранящих фильтры. Спрятать невидимые, показать видимые
	int i = 0;
	for (auto& ci: _columnsToShow) {
		int localX1 = ui->tableView->columnViewportPosition(i);
		int localX2 = ui->tableView->columnViewportPosition(i) + ui->tableView->columnWidth(i);
		if (localX2 < 0 || localX1 >= ui->tableView->size().width()) {
			ci.lineEdit->hide();
		}
		else {
			ci.lineEdit->show();
		}
		if (localX1 < 0) {
			localX1 = 0;
		}
		if (localX2 > ui->tableView->size().width()) {
			localX2 = ui->tableView->size().width();
		}

		int posx1 = ui->tableView->pos().x() + localX1;
		int posx2 = ui->tableView->pos().x() + localX2;
		int posy = ui->tableView->pos().y();
		int sizeY = 20;

		int mainToolBarHeight = ui->mainToolBar->size().height();
		ci.lineEdit->setGeometry(posx1, posy - sizeY + mainToolBarHeight, posx2-posx1, sizeY);
		++i;
	}
}

//---------------------------------------------------------------

const std::string& MainTableModel::GetTextIdByIndex(int index)
{
	if (index < 0 || index >= _textsToShow.size())
	{
		Log("Error: MainTableModel::GetTextIdByIndex: wrong index: " + std::to_string(index));
		return "";
	}
	return _textsToShow[index]->id;
}

