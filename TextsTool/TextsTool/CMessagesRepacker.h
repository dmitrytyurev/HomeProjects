#pragma once
#include <memory>
#include <vector>
#include "mainwindow.h"
//===============================================================================
// ��������� �� ������� ��������� ��������� ��������� � ������� �������, ���������� ��������� ��� ��������
// ����� �������� ������ ������������� � ������� ��������� ��������� ��� ��������
//===============================================================================

class SerializationBuffer;
class DeserializationBuffer;
using DeserializationBufferPtr = std::unique_ptr<DeserializationBuffer>;
using SerializationBufferPtr = std::shared_ptr<SerializationBuffer>;

namespace Repacker
{
    // �������������� ������ �� ������� ��������� "�� ��������" � ������� ������� "�� ��������"
    void RepackMessagesOutToPackets(std::vector<SerializationBufferPtr>& msgsQueueOut, CHttpManager& httpManager);
    // �������������� ������ �� ������� ��������� ������� � ������� ��������� ���������
    void RepackPacketsInToMessages(CHttpManager& httpManager, std::vector<DeserializationBufferPtr>& msgsQueueIn);
};
