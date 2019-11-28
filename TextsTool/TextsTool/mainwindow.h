#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QTextCodec>

#include "../SharedSrc/Shared.h"
#include "DeserializationBuffer.h"
#include "SerializationBuffer.h"
#include "CHttpManager.h"

class TextsDatabase;
using TextsDatabasePtr = std::shared_ptr<TextsDatabase>;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void update();
	void ProcessSync(DeserializationBuffer& buf);
	void ProcessMessageFromServer(const std::vector<uint8_t>& buf);
	void SendRequestSyncMessage();


private:
    Ui::MainWindow *ui = nullptr;
	QTimer *_timer = nullptr;

	TextsDatabasePtr _dataBase;
	CHttpManager _httpManager;
    std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать на сервер
    std::vector<DeserializationBufferPtr> _msgsQueueIn;  // Очередь пришедших от сервера сообщений
	std::vector<int> _textsKeysRefs;  // Указатели на ключи текстов для быстрой сортировки
	std::vector<TextsInterval> _intervals;
};

#endif // MAINWINDOW_H
