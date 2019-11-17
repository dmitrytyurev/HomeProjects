#pragma once
#include <memory>
#include <vector>
#include "mainwindow.h"
//===============================================================================
// Сообщения из очереди исходящих сообщений переносит в очередь пакетов, оптимально объединяя или разбивая
// Также входящие пакеты перекладывает в очередь сообщений объединяя или разбивая
//===============================================================================

class SerializationBuffer;
class DeserializationBuffer;
using DeserializationBufferPtr = std::unique_ptr<DeserializationBuffer>;
using SerializationBufferPtr = std::shared_ptr<SerializationBuffer>;

namespace Repacker
{
    // Перепаковываем данные из очереди сообщений "на отправку" в очередь пакетов "на отправку"
    void RepackMessagesOutToPackets(std::vector<SerializationBufferPtr>& msgsQueueOut, CHttpManager& httpManager);
    // Перепаковываем данные из очереди пришедших пакетов в очередь пришедших сообщений
    void RepackPacketsInToMessages(CHttpManager& httpManager, std::vector<DeserializationBufferPtr>& msgsQueueIn);
};
