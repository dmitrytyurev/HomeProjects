#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <string>

#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "Shared.h"

Ui::MainWindow* debugGlobalUi = nullptr;

uint GetCurrentTimestamp()
{
    QDateTime current = QDateTime::currentDateTime();
    return  current.toTime_t();
}

//---------------------------------------------------------------

void Log(const std::string& str)
{
    FILE* fp = nullptr;
    errno_t err = fopen_s(&fp, "d:\\log.txt", "at");
    if (err != 0) {
        return;
    }
    fprintf(fp, "%s\n", str.c_str());
    fclose(fp);
}

//---------------------------------------------------------------

void ExitMsg(const std::string& message)
{
    Log("Fatal: " + message);
    throw std::exception("Exiting app exception");
}

//---------------------------------------------------------------

CHttpPacket::CHttpPacket(const std::vector<uint8_t>& packetData, Status status) : _packetData(packetData), _status(status)
{
}

//---------------------------------------------------------------

CHttpPacket::CHttpPacket(DeserializationBuffer& request, Status status) : _status(status)
{
    uint32_t remainDataSize = request._buffer.size() - request.offset;
    _packetData.resize(remainDataSize);
    memcpy(_packetData.data(), request._buffer.data() + request.offset, remainDataSize);
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
    LAST_POST_WAS lastTryPostWasCopy = _lastTryPostWas;
    _lastTryPostWas = LAST_POST_WAS::NONE;

    if (_state == STATE::CONNECTING) {
        CallbackConnecting(reply);
    }
    else {
        if (_state == STATE::CONNECTED) {
            if (lastTryPostWasCopy == LAST_POST_WAS::SEND_PACKET) {
                CallbackSendPacket(reply);
            }
            else {
                if (lastTryPostWasCopy == LAST_POST_WAS::REQUEST_PACKET) {
                    CallbackRequestPacket(reply);
                }
                else {
                    Log("Unexpected _lastTryPostWas:" + std::to_string((int)lastTryPostWasCopy));
                }
            }
        }
        else {
            Log("Unexpected _state:" + std::to_string((int)_state));
        }
    }

    reply->deleteLater();
}

//---------------------------------------------------------------

void CHttpManager::DebugLogServerReply(const std::string& logHeader, int size)
{
    Log(logHeader);
    std::string sstr;
    for (int i = 0; i < size; ++i) {
        std::string s = std::to_string(_httpBuf[i]);
        sstr += s;
        sstr += " ";
    }
    Log("  " + sstr);
    //debugGlobalUi->textEdit->setText(QString::fromStdString(sstr));
}

//---------------------------------------------------------------

void CHttpManager::CallbackConnecting(QNetworkReply *reply)
{
    if (reply->error()) {
        _state = STATE::NOT_CONNECTED;
        QString errorString = "network error: " + reply->errorString();
        _lastError = errorString;
        qDebug() << errorString;
        return;
    }

    int sizeReceived = (int)reply->read((char*)_httpBuf.data(), _httpBuf.size());
    DebugLogServerReply("Packet from server (Connecting)", sizeReceived);
    DeserializationBuffer buf(_httpBuf.data(), sizeReceived);
    uint8_t code = buf.GetUint<uint8_t>();
    if (code == (uint8_t)AnswersToClient::WrongLoginOrPassword) {
        _state = STATE::NOT_CONNECTED;
        _lastError = "wrong login or password";
        return;
    }
    if (code == (uint8_t)AnswersToClient::Connected) {
        _state = STATE::CONNECTED;
        _sessionId = buf.GetUint<uint32_t>();
        _sendPacketN = 0;
        _rcvPacketN = 0;
        _timeOfRequestPacket = GetCurrentTimestamp();
        return;
    }
    Log("CallbackConnecting: unexpected response. code:" + std::to_string(code));
}

//---------------------------------------------------------------

void CHttpManager::CallbackSendPacket(QNetworkReply *reply)
{
    if (reply->error()) {
        return;
    }

    int sizeReceived = (int)reply->read((char*)_httpBuf.data(), _httpBuf.size());
    DebugLogServerReply("Packet from server (Send Packet)", sizeReceived);
    DeserializationBuffer buf(_httpBuf.data(), sizeReceived);
    uint8_t code = buf.GetUint<uint8_t>();

    if (code == (uint8_t)AnswersToClient::ClientNotConnected ||
        code == (uint8_t)AnswersToClient::WrongSession ) {
        ConnectInner(); // !!! Ничего, что вызывается из коллбека? Если будет глючить, запоминать необходимость вызова и вызвать в Update
        return;
    }
    if (code == (uint8_t)AnswersToClient::PacketReceived) {
        _lastSuccesPostWas = LAST_POST_WAS::SEND_PACKET;
        if (_packetsOut.empty()) {
            Log("CallbackSendPacket: _packetsOut.empty()");
        }
        else {
            _packetsOut.erase(_packetsOut.begin());
        }
        ++_sendPacketN;
        return;
    }
    Log("CallbackSendPacket unexpected response. code:" + std::to_string(code));
}

//---------------------------------------------------------------

void CHttpManager::CallbackRequestPacket(QNetworkReply *reply)
{
    if (reply->error()) {
        return;
    }

    int sizeReceived = (int)reply->read((char*)_httpBuf.data(), _httpBuf.size());
    DebugLogServerReply("Packet from server (Request Packet)", sizeReceived);
    DeserializationBuffer buf(_httpBuf.data(), sizeReceived);
    uint8_t code = buf.GetUint<uint8_t>();

    if (code == (uint8_t)AnswersToClient::ClientNotConnected ||
        code == (uint8_t)AnswersToClient::WrongSession ) {
        ConnectInner(); // !!! Ничего, что вызывается из коллбека? Если будет глючить, запоминать необходимость вызова и вызвать в Update
        return;
    }
    if (code == (uint8_t)AnswersToClient::NoSuchPacketYet)
    {
        uint32_t timeToNextRequest = buf.GetUint<uint32_t>();
        _timeOfRequestPacket = GetCurrentTimestamp() + timeToNextRequest / 1000;
        return;
    }
    if (code == (uint8_t)AnswersToClient::PacketSent) {
        _lastSuccesPostWas = LAST_POST_WAS::REQUEST_PACKET;
        ++_rcvPacketN;
        uint32_t timeToNextRequest = buf.GetUint<uint32_t>();
        _timeOfRequestPacket = GetCurrentTimestamp() + timeToNextRequest;
        _packetsIn.emplace_back(std::make_shared<CHttpPacket>(buf, CHttpPacket::Status::WAITING_FOR_UNPACKING));
        return;
    }
    Log("CallbackRequestPacket unexpected response. code:" + std::to_string(code));
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

void CHttpManager::ConnectInner()
{
    _state = STATE::CONNECTING;
    SerializationBuffer buf;
    buf.PushStringWithoutZero<uint8_t>(_login);
    buf.PushStringWithoutZero<uint8_t>(_password);
    buf.Push((uint8_t)ClientRequestTypes::RequestConnect);
    SendPacket(buf.buffer);
}

//---------------------------------------------------------------

void CHttpManager::Connect(const std::string& login, const std::string& password)
{
    _login = login;
    _password = password;
    ConnectInner();
}

//---------------------------------------------------------------

void CHttpManager::PutPacketToSendQueue(const std::vector<uint8_t>& packet)
{
    _packetsOut.emplace_back(std::make_shared<CHttpPacket>(packet, CHttpPacket::Status::TO_SEND));
}

//---------------------------------------------------------------

bool CHttpManager::IsTimeToRequestPacket()
{
    return _timeOfRequestPacket != 0 && GetCurrentTimestamp() >= _timeOfRequestPacket;
}

//---------------------------------------------------------------

void CHttpManager::RequestPacket()
{
    _lastTryPostWas = LAST_POST_WAS::REQUEST_PACKET;

    SerializationBuffer buf;
    buf.PushStringWithoutZero<uint8_t>(_login);
    buf.PushStringWithoutZero<uint8_t>(_password);
    buf.Push((uint8_t)ClientRequestTypes::RequestPacket);
    buf.Push(_sessionId);
    buf.Push(_rcvPacketN);
    SendPacket(buf.buffer);
}

//---------------------------------------------------------------

void CHttpManager::SendPacket()
{
    if (_packetsOut.empty()) {
        Log("SendPacket: _packetsOut.empty()");
        return;
    }

    _lastTryPostWas = LAST_POST_WAS::SEND_PACKET;
    SerializationBuffer buf;
    buf.PushStringWithoutZero<uint8_t>(_login);
    buf.PushStringWithoutZero<uint8_t>(_password);
    buf.Push((uint8_t)ClientRequestTypes::ProvidePacket);
    buf.Push(_sessionId);
    buf.Push(_sendPacketN);
    buf.PushBytes(_packetsOut[0]->_packetData.data(), _packetsOut[0]->_packetData.size());
    SendPacket(buf.buffer);
}

//---------------------------------------------------------------

void CHttpManager::Update()
{
    if (_state != STATE::CONNECTED || _lastTryPostWas != LAST_POST_WAS::NONE) {
        return;
    }
    if (_packetsOut.empty()) {
        if (IsTimeToRequestPacket()) {
            RequestPacket();
        }
    }
    else {
        if (IsTimeToRequestPacket()) {
            if (_lastSuccesPostWas == LAST_POST_WAS::SEND_PACKET) {
                RequestPacket();
            }
            else {
                SendPacket();
            }
        }
        else {
            SendPacket();
        }
    }
}

//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);

    ui->setupUi(this);
    debugGlobalUi = ui;
}

//---------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete ui;
}

//---------------------------------------------------------------

void MainWindow::on_pushButton_clicked()
{
    httpManager.Connect("mylogin", "mypassword");
}

//---------------------------------------------------------------

void MainWindow::update()
{
    httpManager.Update();
}
