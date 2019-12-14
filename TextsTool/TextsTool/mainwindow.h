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
using TextsDatabasePtr = std::shared_ptr<TextsDatabase>;

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

	void fillTextsToShowIndices();
	void recalcColumnToShowData();
signals:

public slots:
	void theDataChanged();

private:
	std::string& getDataReference(const QModelIndex &index, bool& isFound);

private:
	TextsDatabasePtr& _dataBase;
	std::vector<TextTranslated*> _textsToShow; // Выборка текстов, которую будем показывать
	std::vector<int> _columnsToShow; // Индексы атрибутов в TextsDatabase::_attributeProps, которые показываем в качестве колонок
};
using MainTableModelPtr = std::unique_ptr<MainTableModel>;

//---------------------------------------------------------------

namespace Ui {
class MainWindow;
}

//---------------------------------------------------------------

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void update();
	void ApplyDiffForSync(DeserializationBuffer& buf);
	void ProcessMessageFromServer(const std::vector<uint8_t>& buf);
	void LoadBaseAndRequestSync(const std::string& dbName);


private:
    Ui::MainWindow *ui = nullptr;
	QTimer *_timer = nullptr;

	TextsDatabasePtr _dataBase;
	CHttpManager _httpManager;
    std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать на сервер
    std::vector<DeserializationBufferPtr> _msgsQueueIn;  // Очередь пришедших от сервера сообщений
	std::vector<int> _textsKeysRefs;  // Указатели на ключи текстов для быстрой сортировки
	std::vector<TextsInterval> _intervals;
	MainTableModelPtr _mainTableModel;  // Модель обеспечивающая доступ к данным главной таблицы текстов
};

#endif // MAINWINDOW_H
