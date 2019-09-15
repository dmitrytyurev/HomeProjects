#pragma once
#include <vector>
#include <stdint.h>
#include <functional>
#include <thread> 
#include <mutex> 

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
	void Update(double dt);

private:
	void ThreadListenSocketFunc();
	void ThreadExitMsg(const std::string& errorMsg);

private:
	std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> _requestCallback;
	bool _isWsaInited = false;
	int _listen_socket = INVALID_SOCKET;
	std::thread _listenSocketThread;
	std::string _fatalErrorInTheradText; // Если не пустая строка, то в потоке произошла фатальная ошибка. Надо напечатать эту ошибку и завершить приложение
	std::mutex _fatalErrorInTheradTextMutex;
	bool _isRequestThreadToFinish = false; // Основной поток просит неосновной поток завершиться (при выходе)
};