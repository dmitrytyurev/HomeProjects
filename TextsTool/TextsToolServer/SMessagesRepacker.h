#pragma once

//===============================================================================
// ��������� �� ������� ��������� ��������� ��������� � ������� �������, ���������� ��������� ��� ��������
// ����� �������� ������ ������������� � ������� ��������� ��������� ��� ��������
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
	void RepackMessagesOutToPackets(std::shared_ptr<SConnectedClient>& client, SConnectedClientLow* clLow); // �������������� ������ �� ������� ��������� "�� ��������" � ������� ������� "�� ��������"
	void RepackPacketsInToMessages(std::shared_ptr<SConnectedClient>& client, SConnectedClientLow* clLow);  // �������������� ������ �� ������� ��������� ������� � ������� ��������� ���������

	STextsToolApp* _app = nullptr;
};
