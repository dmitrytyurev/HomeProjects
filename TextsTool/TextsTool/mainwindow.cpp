#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include "SerializationBuffer.h"
#include "Shared.h"

Ui::MainWindow* debugGlobalUi = nullptr;

uint GetCurrentTimestamp()
{
    QDateTime current = QDateTime::currentDateTime();
    return  current.toTime_t();
}

//---------------------------------------------------------------


CHttpManager::CHttpManager()
{
    _httpBuf.resize(HTTP_BUF_SIZE);
    _manager = new QNetworkAccessManager();
    QObject::connect(_manager, &QNetworkAccessManager::finished, this, std::bind(&CHttpManager::HttpRequestFinishedCallback, this, std::placeholders::_1));

}

//---------------------------------------------------------------

CHttpManager::~CHttpManager()
{
    delete _manager;
}

//---------------------------------------------------------------

void CHttpManager::HttpRequestFinishedCallback(QNetworkReply *reply)
{
    if (_state == STATE::CONNECTING) {
        CallbackConnecting(reply);
    }
    else {
        if (_state == STATE::CONNECTED) {
            if (_lastTryPostWas == LAST_POST_WAS::SEND_PACKET) {
                CallbackSendPacket(reply);
            }
            else {
                if (_lastTryPostWas == LAST_POST_WAS::REQUEST_PACKET) {
                    CallbackRequestPacket(reply);
                }
            }
        }
    }

    reply->deleteLater();
}

//---------------------------------------------------------------

void CHttpManager::DebugPrintServerReply(int size)
{
    std::string sstr;
    for (int i = 0; i < size; ++i) {
        std::string s = std::to_string(_httpBuf[i]);
        sstr += s;
        sstr += " ";
    }
    debugGlobalUi->textEdit->setText(QString::fromStdString(sstr));
}

//---------------------------------------------------------------

void CHttpManager::CallbackConnecting(QNetworkReply *reply)
{
    _lastTryPostWas = LAST_POST_WAS::NONE;
    if (reply->error()) {
        _state = STATE::NOT_CONNECTED;
        QString errorString = "network error: " + reply->errorString();
        _lastError = errorString;
        qDebug() << errorString;
        return;
    }

    qint64 size = reply->read((char*)_httpBuf.data(), _httpBuf.size());
    DebugPrintServerReply(size);

    if (connect_success) {
        _state = STATE::CONNECTED;
        _sessionId = из пакета
        _sendPacketN = 0;
        _rcvPacketN = 0;
        _timeOfRequestPacket = GetCurrentTimestamp();
    }
    else {
        _state = STATE::NOT_CONNECTED;
        _lastError = "wrong login or password";
    }
}

//---------------------------------------------------------------

void CHttpManager::CallbackSendPacket(QNetworkReply *reply)
{

}

//---------------------------------------------------------------

void CHttpManager::CallbackRequestPacket(QNetworkReply *reply)
{

}

//---------------------------------------------------------------

void CHttpManager::SendPacket(const std::vector<uint8_t>& packet)
{
    QByteArray postData((const char*)packet.data(), packet.size());

    _request.setUrl(QUrl("http://127.0.0.1:8000/"));

    _request.setRawHeader("User-Agent", "My app name v0.1");
    _request.setRawHeader("X-Custom-User-Agent", "My app name v0.1");
    _request.setRawHeader("Content-Type", "application/json");
    QByteArray postDataSize = QByteArray::number(packet.size());
    _request.setRawHeader("Content-Length", postDataSize);
    _manager->post(_request, postData);
}

//---------------------------------------------------------------

void CHttpManager::TestSend()
{
    std::vector<uint8_t> packet = {1, 2, 3};
    SendPacket(packet);
}

//---------------------------------------------------------------

void CHttpManager::Connect(const std::string& login, const std::string& password)
{
    _state = STATE::CONNECTING;
    SerializationBuffer buf;
    buf.PushStringWithoutZero<uint8_t>(login);
    buf.PushStringWithoutZero<uint8_t>(password);
    buf.Push((uint8_t)ClientRequestTypes::RequestConnect);
    SendPacket(buf.buffer);
}



//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    debugGlobalUi = ui;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    httpManager.Connect("mylogin", "mypassword");
}
