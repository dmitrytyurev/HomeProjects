#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <string>

#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "../SharedSrc/Shared.h"
#include "CMessagesRepacker.h"
#include "Utils.h"

Ui::MainWindow* debugGlobalUi = nullptr;

//---------------------------------------------------------------

void MakeKey(uint32_t tsModified, const std::string& textId, std::vector<uint8_t>& result)
{
	result.resize(sizeof(uint32_t) + textId.size());
	uint8_t* p = &result[0];

	*(reinterpret_cast<uint32_t*>(p)) = tsModified;
	memcpy(p + sizeof(uint32_t), textId.c_str(), textId.size());
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
    std::string sstr = logHeader + ": ";
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
	uint8_t code = buf.GetUint8();
    if (code == (uint8_t)AnswersToClient::WrongLoginOrPassword) {
        _state = STATE::NOT_CONNECTED;
        _lastError = "wrong login or password";
        return;
    }
    if (code == (uint8_t)AnswersToClient::Connected) {
        _state = STATE::CONNECTED;
		_sessionId = buf.GetUint32();
        _sendPacketN = 0;
        _rcvPacketN = 0;
		_timeOfRequestPacket = Utils::GetCurrentTimestamp();
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
	uint8_t code = buf.GetUint8();

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
	uint8_t code = buf.GetUint8();

    if (code == (uint8_t)AnswersToClient::ClientNotConnected ||
        code == (uint8_t)AnswersToClient::WrongSession ) {
        ConnectInner(); // !!! Ничего, что вызывается из коллбека? Если будет глючить, запоминать необходимость вызова и вызвать в Update
        return;
    }
    if (code == (uint8_t)AnswersToClient::NoSuchPacketYet)
    {
		uint32_t timeToNextRequest = buf.GetUint32();
		_timeOfRequestPacket = Utils::GetCurrentTimestamp() + timeToNextRequest / 1000;
        return;
    }
    if (code == (uint8_t)AnswersToClient::PacketSent) {
        _lastSuccesPostWas = LAST_POST_WAS::REQUEST_PACKET;
        ++_rcvPacketN;
		uint32_t timeToNextRequest = buf.GetUint32();
		_timeOfRequestPacket = Utils::GetCurrentTimestamp() + timeToNextRequest;
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
    Log("SendPacket");
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
	buf.PushString8(_login);
	buf.PushString8(_password);
	buf.PushUint8(ClientRequestTypes::RequestConnect);
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
	return _timeOfRequestPacket != 0 && Utils::GetCurrentTimestamp() >= _timeOfRequestPacket;
}

//---------------------------------------------------------------

void CHttpManager::RequestPacket()
{
    _lastTryPostWas = LAST_POST_WAS::REQUEST_PACKET;

    SerializationBuffer buf;
	buf.PushString8(_login);
	buf.PushString8(_password);
	buf.PushUint8(ClientRequestTypes::RequestPacket);
	buf.PushUint32(_sessionId);
	buf.PushUint32(_rcvPacketN);
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
	buf.PushString8(_login);
	buf.PushString8(_password);
	buf.PushUint8(ClientRequestTypes::ProvidePacket);
	buf.PushUint32(_sessionId);
	buf.PushUint32(_sendPacketN);
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
    Log("\n\n=== Start App ===========================================================");
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(50);

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
	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());

/*
	_msgsQueueOut.back()->PushUint8(EventType::RequestSync);
	_msgsQueueOut.back()->PushString8("TestDB");
	_msgsQueueOut.back()->PushUint32(2); // Число папок


	_msgsQueueOut.back()->PushUint32(55443); // id папки
	_msgsQueueOut.back()->PushUint32(50000); // TS изменения папки на клиенте
	_msgsQueueOut.back()->PushUint32(0); // Количество отобранных ключей


	_msgsQueueOut.back()->PushUint32(11111); // id папки
	_msgsQueueOut.back()->PushUint32(54321); // TS изменения папки на клиенте
	_msgsQueueOut.back()->PushUint32(0); // Количество отобранных ключей
*/

	_msgsQueueOut.back()->PushUint8(EventType::RequestSync);
	_msgsQueueOut.back()->PushString8("TestDB");
	_msgsQueueOut.back()->PushUint32(1); // Число папок


	_msgsQueueOut.back()->PushUint32(55443322); // id папки
	_msgsQueueOut.back()->PushUint32(50000); // TS изменения папки на клиенте
	_msgsQueueOut.back()->PushUint32(3); // Количество отобранных ключей

	struct Text
	{
		std::string id;
		uint32_t ts;
		std::vector<uint8_t> key;
	};

	std::vector<Text> texts = {{"TextID1", 101}, {"TextID2", 102}, {"TextID3", 103}, {"TextID4", 104}, {"TextID5", 105}, {"TextID6", 106}, {"TextID7", 107}, {"TextID8", 108}, {"TextID9", 109}, {"TextID10", 110}};

	for (auto& text: texts) {
		MakeKey(text.ts, text.id, text.key);
	}

	// Ключи разбиения текстов
	_msgsQueueOut.back()->PushUint8(texts[2].key.size());
	_msgsQueueOut.back()->PushBytes(texts[2].key.data(), texts[2].key.size());

	_msgsQueueOut.back()->PushUint8(texts[4].key.size());
	_msgsQueueOut.back()->PushBytes(texts[4].key.data(), texts[4].key.size());

	_msgsQueueOut.back()->PushUint8(texts[7].key.size());
	_msgsQueueOut.back()->PushBytes(texts[7].key.data(), texts[7].key.size());

//Log("Key:");
//Utils::LogBuf(texts[1].key);
	// Инфа об интервалах (на 1 больше числа ключей)
	_msgsQueueOut.back()->PushUint32(2); // Число текстов в интервале
	uint64_t hash = 0;
	hash = Utils::AddHash(hash, texts[0].key, true);
	hash = Utils::AddHash(hash, texts[1].key, false);
	_msgsQueueOut.back()->PushBytes(&hash, sizeof(uint64_t));

	_msgsQueueOut.back()->PushUint32(2); // Число текстов в интервале
	hash = Utils::AddHash(hash, texts[2].key, true);
	hash = Utils::AddHash(hash, texts[3].key, false);
	_msgsQueueOut.back()->PushBytes(&hash, sizeof(uint64_t));

	_msgsQueueOut.back()->PushUint32(3); // Число текстов в интервале
	hash = Utils::AddHash(hash, texts[4].key, true);
	hash = Utils::AddHash(hash, texts[5].key, false);
	hash = Utils::AddHash(hash, texts[6].key, false);
	_msgsQueueOut.back()->PushBytes(&hash, sizeof(uint64_t));

	_msgsQueueOut.back()->PushUint32(3); // Число текстов в интервале
	hash = Utils::AddHash(hash, texts[7].key, true);
	hash = Utils::AddHash(hash, texts[8].key, false);
	hash = Utils::AddHash(hash, texts[9].key, false);
	_msgsQueueOut.back()->PushBytes(&hash, sizeof(uint64_t));

	//-------------------

	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	_msgsQueueOut.back()->PushUint8(111); // Изменить основной текст
	_msgsQueueOut.back()->PushString8("TextID1");
	_msgsQueueOut.back()->PushString16("NewBaseText1");




	//-------------------

    httpManager.Connect("mylogin", "mypassword");
}

//---------------------------------------------------------------

void MainWindow::update()
{
	httpManager.Update();
	Repacker::RepackPacketsInToMessages(httpManager, _msgsQueueIn);
	Repacker::RepackMessagesOutToPackets(_msgsQueueOut, httpManager);

	for (auto& msg: _msgsQueueIn) {
		Log("Message:  ");
		Utils::LogBuf(msg->_buffer);
	}
	_msgsQueueIn.resize(0);
}
