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
	SHttpManagerLowImpl(std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> requestCallback); // Запоминает функцию, которую будет дёргать при каждом входящем http-запросе (неосновной поток!). Функция должна сформировать ответ на запрос
	void StartHttpListening();
	~SHttpManagerLowImpl();

private:
	std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> _requestCallback;
	bool _isWsaInited = false;
	struct addrinfo* _addr = nullptr; // структура, хранящая информацию об IP-адресе  слущающего сокета
	int _listen_socket = INVALID_SOCKET;
};