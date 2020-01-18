#pragma once
#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QTextCodec>
#include <QAbstractTableModel>

#include "../SharedSrc/Shared.h"
#include "../SharedSrc/DeserializationBuffer.h"
#include "../SharedSrc/SerializationBuffer.h"
#include "TextsBaseClasses.h"
#include "CHttpManager.h"

namespace Ui {
	class MainWindow;
}
class TextsDatabase;
using TextsDatabasePtr = std::shared_ptr<TextsDatabase>;
struct FoundTextRefs;
class DatabaseManager;
class QLineEdit;

//---------------------------------------------------------------

enum class TEXTS_RECOLLECT_TYPE
{
	YES,
	NO,
	IF_COLUMNS_AFFECTED
};


//---------------------------------------------------------------
class MainTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit MainTableModel(TextsDatabasePtr& dataBase);
	int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole) Q_DECL_OVERRIDE;
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

	QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
	void OnDataModif(bool columnsChanged, TEXTS_RECOLLECT_TYPE textsRecollectType,  std::vector<AttributeProperty*>* affectedAttributes, bool sortTypeChanged, int line);
	int calcLineByTextId(const std::string& textId);
	int calcColumnByType(uint8_t attribType);
	AttributeProperty* getAttributeByType(uint8_t attribType);
	AttributeProperty* getAttributeById(int attributId);
	void Update(Ui::MainWindow* ui);
	const std::string& GetTextIdByIndex(int index);

signals:

public slots:
	void theDataChanged();
	void filterEditFinished();

private:
	bool getTextReferences(const QModelIndex &index, bool needCreateAttrIfNotFound, FoundTextRefs& result);
	void recollectTextsFromSelectedFolder();
	// Заполнить ссылки на тексты, которые должны показывать (учитывая выбранную папку, начиная с которой показываем тексты и применённые фильтры).
	void addFolderTextsToShowReq(uint32_t parentId, std::vector<AttributeProperty*>& attribsFilter);
	bool ifTextPassesFilters(TextTranslatedPtr& text, std::vector<AttributeProperty*>& attribsFilter);
	void SortTextsByCurrentSortType();
	void SortTextsById();
	void SortTextsByIdBack();
	void SortTextsByCreateTime();
	void SortTextsByCreateTimeBack();
	void SortTextsByModifyTime();
	void SortTextsByModifyTimeBack();
	void SortTextsByLoginOfModifier();
	void SortTextsByLoginOfModifierBack();
	void recalcColumnToShowData();

private:
	struct COLUMN_INFO
	{
		int attribIndex;  // Индекс атрибута в TextsDatabase::_attributeProps, который показываем в качестве текущей колонки
		std::unique_ptr<QLineEdit> lineEdit;  // Созданный контрол LineEdit, отображающийся над данной колонкой таблицы и служащий для ввода фильтра по данной колонке
	};

	TextsDatabasePtr& _dataBase;
	bool _isFiltersOn = false;   // Включены ли фильтры главной таблицы
	std::vector<TextTranslated*> _textsToShow; // Выборка текстов, которую будем показывать
	std::vector<COLUMN_INFO> _columnsToShow; // См. описание полей COLUMN_INFO
};
using MainTableModelPtr = std::unique_ptr<MainTableModel>;
