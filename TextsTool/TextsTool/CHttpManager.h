#pragma once

#include <QObject>
#include <QNetworkReply>
#include "../SharedSrc/DeserializationBuffer.h"
#include "../SharedSrc/SerializationBuffer.h"


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
	static void Init();
	static void Deinit();
	static CHttpManager& Instance();
	void Update(int dtMs);
	void Connect(const std::string& login, const std::string& password, std::function<void (bool isSuccess)> callback);
    void TestSend();
    STATE GetStatus();
    void PutPacketToSendQueue(const std::vector<uint8_t>& packet);
	const std::string& GetLogin();

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

	static CHttpManager* pthis;
	std::vector<CHttpPacket::Ptr> _packetsOut; // Пакеты для отсылки на сервер
    STATE _state = STATE::NOT_CONNECTED;
    QString _lastError;
    uint32_t _sendPacketN = 0; // Нумерация пакетов, которые клиент посылает на сервер
    uint32_t _rcvPacketN = 0;  // Нумерация пакетов, которые клиент запрашивает у сервера (номер пакета, который будет запрашивать у сервера следующим)
    uint32_t _sessionId = 0;
    LAST_POST_WAS _lastTryPostWas = LAST_POST_WAS::NONE;
    LAST_POST_WAS _lastSuccesPostWas = LAST_POST_WAS::NONE;
	int _timeToRequestPacket = 0;  // Время в миллисекундах через сколько нужно послать запрос следующего пакета с сервера
	int _requestTimout = 0; // Таймаут между запросами пакетов с сервера. Может уменьшаться и увеличиваться в зависимости от активности отсылаемых и получаемых пакетов
    QNetworkAccessManager* _manager = nullptr;
    QNetworkRequest _request;
    std::vector<uint8_t> _httpBuf;
    std::string _login;
    std::string _password;
	std::function<void (bool isSuccess)> _connectCallback;
};
