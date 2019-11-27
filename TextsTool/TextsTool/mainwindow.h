#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QTextCodec>

#include "DeserializationBuffer.h"
#include "SerializationBuffer.h"
#include "CHttpManager.h"

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

private:
    Ui::MainWindow *ui = nullptr;
    QTimer *timer = nullptr;
    CHttpManager httpManager;
    std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать на сервер
    std::vector<DeserializationBufferPtr> _msgsQueueIn;  // Очередь пришедших от сервера сообщений
};

#endif // MAINWINDOW_H
