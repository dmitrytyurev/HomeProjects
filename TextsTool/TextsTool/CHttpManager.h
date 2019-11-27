#pragma once

#include "DeserializationBuffer.h"
#include "SerializationBuffer.h"


class CHttpPacket
{
public:
    using Ptr = std::shared_ptr<CHttpPacket>;

    enum class Status
    {
        // ---- Для пакетов подлежащих отсылке на сервер
        TO_SEND,
        // ---- Для пакетов пришедших с сервера
        WAITING_FOR_UNPACKING,
        UNPACKING,
        UNPACKED
    };

    CHttpPacket(const std::vector<uint8_t>& packetData, Status status);
    CHttpPacket(DeserializationBuffer& request, Status status);

    std::vector<uint8_t> _packetData;
    Status _status;
};

//---------------------------------------------------------------


class CHttpManager: public QObject
{
public:
    enum class STATE
    {
        NOT_CONNECTED,
        CONNECTING,
        CONNECTED,
    };
    enum class LAST_POST_WAS
    {
       NONE,
       SEND_PACKET,
       REQUEST_PACKET
    };

    CHttpManager();
    ~CHttpManager();
    void Update();
    void Connect(const std::string& login, const std::string& password);
    void TestSend();
    STATE GetStatus();
    void PutPacketToSendQueue(const std::vector<uint8_t>& packet);

public:
    std::vector<CHttpPacket::Ptr> _packetsIn; // Пакеты, полученные с сервера. Их обработает Repacker, перепакует и положит в очередь входящих сообщений

private:
    void ConnectInner();
    void HttpRequestFinishedCallback(QNetworkReply *reply);
    void CallbackConnecting(QNetworkReply *reply);
    void CallbackSendPacket(QNetworkReply *reply);
    void CallbackRequestPacket(QNetworkReply *reply);
    void SendPacket(const std::vector<uint8_t>& packet);
    void DebugLogServerReply(const std::string& logHeader, int size);
    bool IsTimeToRequestPacket();
    void RequestPacket();
    void SendPacket();

    std::vector<CHttpPacket::Ptr> _packetsOut; // Пакеты для отсылки на сервер
    STATE _state = STATE::NOT_CONNECTED;
    QString _lastError;
    uint32_t _sendPacketN = 0; // Нумерация пакетов, которые клиент посылает на сервер
    uint32_t _rcvPacketN = 0;  // Нумерация пакетов, которые клиент запрашивает у сервера (номер пакета, который будет запрашивать у сервера следующим)
    uint32_t _sessionId = 0;
    LAST_POST_WAS _lastTryPostWas = LAST_POST_WAS::NONE;
    LAST_POST_WAS _lastSuccesPostWas = LAST_POST_WAS::NONE;
    uint _timeOfRequestPacket = 0;  // Время следующего запроса пакета с сервера
    QNetworkAccessManager* _manager = nullptr;
    QNetworkRequest _request;
    std::vector<uint8_t> _httpBuf;
    std::string _login;
    std::string _password;
};
