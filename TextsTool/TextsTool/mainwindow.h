#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
class MainWindow;
using TextsDatabasePtr = std::shared_ptr<TextsDatabase>;


//---------------------------------------------------------------
struct FoundTextRefs
{
	TextTranslated* text = nullptr;
	AttributeInText* attrInText = nullptr;
	AttributeProperty* attrInTable = nullptr;
	std::string* string = nullptr;
	bool wasAttrInTextCreated = false;
};

//---------------------------------------------------------------
class DatabaseManager;
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

//---------------------------------------------------------------

class DatabaseManager
{
public:
	DatabaseManager ();
	static void Init();
	static void Deinit();
	static DatabaseManager& Instance();
	void LoadBaseAndRequestSync(const std::string& dbName);
	void OnTextModifiedFromGUI(const FoundTextRefs& textRefs);
	void ProcessMessageFromServer(const std::vector<uint8_t>& buf);
	void SaveDatabase();
	void Update();

private:
	void ApplyDiffForSync(DeserializationBuffer& buf);
	std::string ModifyDbChangeBaseText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifie);
	void SendMsgChangeBaseText(const FoundTextRefs& textRefs);
	void SendMsgChangeTextAttrib(const FoundTextRefs& textRefs);
	void ResetTextAndFolderTimestamps(const FoundTextRefs& textRefs);

	static DatabaseManager* pthis;
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать на сервер
	std::vector<DeserializationBufferPtr> _msgsQueueIn;  // Очередь пришедших от сервера сообщений
	TextsDatabasePtr _dataBase;
	MainTableModelPtr _mainTableModel;  // Модель обеспечивающая доступ к данным главной таблицы текстов
	std::vector<int> _textsKeysRefs;  // Указатели на ключи текстов для быстрой сортировки
	std::vector<TextsInterval> _intervals;
};

//---------------------------------------------------------------

namespace Ui {
class MainWindow;
}

//---------------------------------------------------------------

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
	static MainWindow& Instance() { return *pthis; }
	explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
	void closeEvent (QCloseEvent *event);
	void SetModelForMainTable(QAbstractTableModel* model);

private slots:
	void on_pushButton_clicked();
	void update();

private:
    Ui::MainWindow *ui = nullptr;
	QTimer *_timer = nullptr;

	static MainWindow* pthis;
};

#endif // MAINWINDOW_H
