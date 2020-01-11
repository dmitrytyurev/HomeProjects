#include <QTimer>
#include <string>
#include <algorithm>

#include "../SharedSrc/SerializationBuffer.h"
#include "../SharedSrc/DeserializationBuffer.h"
#include "../SharedSrc/Shared.h"
#include "CMessagesRepacker.h"
#include "Utils.h"
#include "CHttpManager.h"
#include <QElapsedTimer>


const static int MinTimeoutRequestPacketFromServer = 150;  // На сколько миллисикунд сбрасывается таймаут запроса пакетов на сервере при активности (посылка данных на сервер или получение данных с сервера)
const static int MaxTimeoutRequestPacketFromServer = 3000; // Максимальный таймаут запроса пакетов на сервере (он плавно поднимается до этого значения при отсутствии активности)
const static float IncreaseTimeoutRequestPacketFromServer = 1.05f; // Множитель для плавного увеличения таймаута запроса пакетов на сервере (при отсутствии активности)

extern QElapsedTimer gTimer;
CHttpManager* CHttpManager::pthis = nullptr;

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

void CHttpManager::Init()
{
	if (pthis) {
		ExitMsg("CHttpManager::Init: already inited");
	}
	pthis = new CHttpManager;
}

//---------------------------------------------------------------

void CHttpManager::Deinit()
{
	if (pthis) {
		delete pthis;
		pthis = nullptr;
	}
}

//---------------------------------------------------------------

CHttpManager& CHttpManager::Instance()
{
	if (!pthis) {
		ExitMsg("CHttpManager::Instance: not inited");
	}
	return *pthis;
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
		_connectCallback(false);
        return;
    }

    int sizeReceived = (int)reply->read((char*)_httpBuf.data(), _httpBuf.size());
	//DebugLogServerReply("Packet from server (Connecting)", sizeReceived);
    DeserializationBuffer buf(_httpBuf.data(), sizeReceived);
	uint8_t code = buf.GetUint8();
    if (code == (uint8_t)AnswersToClient::WrongLoginOrPassword) {
        _state = STATE::NOT_CONNECTED;
        _lastError = "wrong login or password";
		_connectCallback(false);
		return;
    }
    if (code == (uint8_t)AnswersToClient::Connected) {
        _state = STATE::CONNECTED;
		_sessionId = buf.GetUint32();
        _sendPacketN = 0;
        _rcvPacketN = 0;
		_requestTimout = MinTimeoutRequestPacketFromServer;
		_connectCallback(true);
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
	//DebugLogServerReply("Packet from server (Send Packet)", sizeReceived);
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
	//DebugLogServerReply("Packet from server (Request Packet)", sizeReceived);
    DeserializationBuffer buf(_httpBuf.data(), sizeReceived);
	uint8_t code = buf.GetUint8();

    if (code == (uint8_t)AnswersToClient::ClientNotConnected ||
        code == (uint8_t)AnswersToClient::WrongSession ) {
        ConnectInner(); // !!! Ничего, что вызывается из коллбека? Если будет глючить, запоминать необходимость вызова и вызвать в Update
        return;
    }
    if (code == (uint8_t)AnswersToClient::NoSuchPacketYet)
    {
        return;
    }
    if (code == (uint8_t)AnswersToClient::PacketSent) {
		_requestTimout = MinTimeoutRequestPacketFromServer;
        _lastSuccesPostWas = LAST_POST_WAS::REQUEST_PACKET;
        ++_rcvPacketN;
		bool isNextPacketReady = static_cast<bool>(buf.GetUint8());
		if (isNextPacketReady) {
			_timeToRequestPacket = 0;
		}
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
	//Log("SendPacket");
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

void CHttpManager::Connect(const std::string& login, const std::string& password, std::function<void (bool isSuccess)> callback)
{
    _login = login;
    _password = password;
	_connectCallback = callback;
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
	return _timeToRequestPacket == 0;
}

//---------------------------------------------------------------

void CHttpManager::RequestPacket()
{
	_timeToRequestPacket = _requestTimout;
	_requestTimout = std::min(static_cast<int>(_requestTimout * IncreaseTimeoutRequestPacketFromServer), MaxTimeoutRequestPacketFromServer);
//Log("RequestPacket, _requestTimout: " + std::to_string(_requestTimout));
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

	_requestTimout = MinTimeoutRequestPacketFromServer;
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

void CHttpManager::Update(int dtMs)
{
	_timeToRequestPacket = std::max(_timeToRequestPacket - dtMs, 0);

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

const std::string& CHttpManager::GetLogin()
{
	return _login;
}
