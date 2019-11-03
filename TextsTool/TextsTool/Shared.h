const int HTTP_BUF_SIZE = 500000;

enum class ClientRequestTypes // Типы запросов от клиента
{
    RequestConnect,
    RequestPacket,
    ProvidePacket
};

enum class AnswersToClient // Коды ответов
{
    UnknownRequest,
    WrongLoginOrPassword,
    WrongSession,
    ClientNotConnected,
    NoSuchPacketYet,
    Connected,
    PacketReceived,
    PacketSent
};
