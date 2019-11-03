#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QTextCodec>

class CHttpPacket
{
public:
    using Ptr = std::shared_ptr<CHttpPacket>;

    enum class Status
    {
        RECEIVED,
        WAITING_FOR_UNPACKING,
        UNPAKING,
        UNPACKED
    };

    CHttpPacket(std::vector<uint8_t>& packetData, Status status);

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
    void Connect(const std::string& login, const std::string& password);
    void TestSend();
    STATE GetStatus();
    void PutPacketToSendQueue(const CHttpPacket& packet);

public:
    std::vector<CHttpPacket::Ptr> _packetsIn;

private:
    void HttpRequestFinishedCallback(QNetworkReply *reply);
    void CallbackConnecting(QNetworkReply *reply);
    void CallbackSendPacket(QNetworkReply *reply);
    void CallbackRequestPacket(QNetworkReply *reply);
    void SendPacket(const std::vector<uint8_t>& packet);
    void DebugPrintServerReply(int size);

    std::vector<CHttpPacket::Ptr> _packetsOut;
    STATE _state = STATE::NOT_CONNECTED;
    QString _lastError;
    int _sendPacketN = 0;
    int _rcvPacketN = 0;
    int _sessionId = 0;
    LAST_POST_WAS _lastTryPostWas = LAST_POST_WAS::NONE;
    LAST_POST_WAS _lastSuccesPostWas = LAST_POST_WAS::NONE;
    uint _timeOfRequestPacket = 0;
    QNetworkAccessManager* _manager = nullptr;
    QNetworkRequest _request;
    std::vector<uint8_t> _httpBuf;
};

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

private:
    Ui::MainWindow *ui;

    CHttpManager httpManager;
};

#endif // MAINWINDOW_H
