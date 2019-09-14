#pragma once
#include <vector>
#include <stdint.h>
#include <functional>

#include <WinSock2.h>


//===============================================================================
//
//===============================================================================
class SHttpManagerLowImpl
{
public:
	SHttpManagerLowImpl(std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> requestCallback); // ���������� �������, ������� ����� ������ ��� ������ �������� http-������� (���������� �����!). ������� ������ ������������ ����� �� ������
	void StartHttpListening();
	~SHttpManagerLowImpl();

private:
	std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> _requestCallback;
	bool _isWsaInited = false;
	struct addrinfo* _addr = nullptr; // ���������, �������� ���������� �� IP-������  ���������� ������
	int _listen_socket = INVALID_SOCKET;
};