#pragma once

//===============================================================================
// Сообщения из очереди исходящих сообщений переносит в очередь пакетов, оптимально объединяя или разбивая
// Также входящие пакеты перекладывает в очередь сообщений объединяя или разбивая
//===============================================================================

class STextsToolApp;
class SConnectedClientLow;
class SConnectedClient;
class DeserializationBuffer;

class SMessagesRepacker
{
public:
	SMessagesRepacker(STextsToolApp* app);
	void Update(double dt);

private:
	void RepackMessagesOutToPackets(std::shared_ptr<SConnectedClient>& client, SConnectedClientLow* clLow); // Перепаковываем данные из очереди сообщений "на отправку" в очередь пакетов "на отправку"
	void RepackPacketsInToMessages(std::shared_ptr<SConnectedClient>& client, SConnectedClientLow* clLow);  // Перепаковываем данные из очереди пришедших пакетов в очередь пришедших сообщений

	STextsToolApp* _app = nullptr;
};
