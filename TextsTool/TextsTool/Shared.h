const int HTTP_BUF_SIZE = 500000;

enum class ClientRequestTypes // ���� �������� �� �������
{
    RequestConnect,
    RequestPacket,
    ProvidePacket
};

enum class AnswersToClient // ���� �������
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
