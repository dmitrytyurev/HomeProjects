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

class TextsDatabase;
using TextsDatabasePtr = std::shared_ptr<TextsDatabase>;
struct FoundTextRefs;
class DatabaseManager;

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

	// Заполнить ссылки на тексты, которые должны показывать (учитывая выбранную папку, начиная с которой показываем тексты и применённые фильтры).
	// Если justOneTextContentChanged == true, то перезаполняем индексы только, если включены фильтры таблицы. Иначе сразу выход
	void fillRefsToTextsToShow();
	void recalcColumnToShowData();
	void OnDataModif(bool oneCellChanged, bool columnsCanChange, int line, int column);
	int calcLineByTextId(const std::string& textId);
	int calcColumnOfBaseText();
	int calcColumnOfAttributInText(int attributId);
signals:

public slots:
	void theDataChanged();

private:
	bool getTextReferences(const QModelIndex &index, bool needCreateAttrIfNotFound, FoundTextRefs& result);

private:
	TextsDatabasePtr& _dataBase;
	bool _isFiltersOn = false;   // Включены ли фильтры главной таблицы
	std::vector<TextTranslated*> _textsToShow; // Выборка текстов, которую будем показывать
	std::vector<int> _columnsToShow; // Индексы атрибутов в TextsDatabase::_attributeProps, которые показываем в качестве колонок
};
using MainTableModelPtr = std::unique_ptr<MainTableModel>;
