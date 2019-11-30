#include <iostream>
#include <sstream>
#include <string>

#include <WS2tcpip.h>

#include "SHttpManagerLowImpl.h"
#include "Utils.h"
#include "../SharedSrc/Shared.h"

// Необходимо, чтобы линковка происходила с DLL-библиотекой
// Для работы с сокетам
#pragma comment(lib, "Ws2_32.lib")


// !!! При конвертации сообщения в пакеты должны резать, если сообщение больше, чем это число

void ExitMsg(const std::string& message);

//===============================================================================

SHttpManagerLowImpl::SHttpManagerLowImpl(std::function<void(DeserializationBuffer&, SerializationBuffer&)> requestCallback) :
	_requestCallback(requestCallback)
{
}

void SHttpManagerLowImpl::StartHttpListening()
{
	WSADATA wsaData; // служебная структура для хранение информации
	// о реализации Windows Sockets
	// старт использования библиотеки сокетов процессом
	// (подгружается Ws2_32.dll)
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	// Если произошла ошибка подгрузки библиотеки
	if (result != 0) {
		ExitMsg("WSAStartup failed: " + std::to_string(result));
	}
	_isWsaInited = true;

	// Шаблон для инициализации структуры адреса
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET; // AF_INET определяет, что будет
	// использоваться сеть для работы с сокетом
	hints.ai_socktype = SOCK_STREAM; // Задаем потоковый тип сокета
	hints.ai_protocol = IPPROTO_TCP; // Используем протокол TCP
	hints.ai_flags = AI_PASSIVE; // Сокет будет биндиться на адрес, чтобы принимать входящие соединения

	struct addrinfo* _addr = nullptr; // структура, хранящая информацию об IP-адресе  слущающего сокета
	// Инициализируем структуру, хранящую адрес сокета - addr
	// Наш HTTP-сервер будет висеть на 8000-м порту локалхоста
	result = getaddrinfo("127.0.0.1", "8000", &hints, &_addr);

	// Если инициализация структуры адреса завершилась с ошибкой,
	// выведем сообщением об этом и завершим выполнение программы
	if (result != 0) {
		ExitMsg("getaddrinfo failed: " + std::to_string(result));
	}

	// Создание сокета
	_listen_socket = socket(_addr->ai_family, _addr->ai_socktype, _addr->ai_protocol);
	// Если создание сокета завершилось с ошибкой, выводим сообщение,
	// освобождаем память, выделенную под структуру addr,
	// выгружаем dll-библиотеку и закрываем программу
	if (_listen_socket == INVALID_SOCKET) {
		freeaddrinfo(_addr);
		ExitMsg("Error at socket: " + std::to_string(WSAGetLastError()));
	}

	// Привязываем сокет к IP-адресу
	result = bind(_listen_socket, _addr->ai_addr, (int)_addr->ai_addrlen);
	freeaddrinfo(_addr);

	// Если привязать адрес к сокету не удалось, то выводим сообщение
	// об ошибке, освобождаем память, выделенную под структуру addr.
	// и закрываем открытый сокет.
	// Выгружаем DLL-библиотеку из памяти и закрываем программу.
	if (result == SOCKET_ERROR) {
		ExitMsg("bind failed with error: " + std::to_string(WSAGetLastError()));
	}

	// Инициализируем слушающий сокет
	if (listen(_listen_socket, SOMAXCONN) == SOCKET_ERROR) {
		ExitMsg("listen failed with error: " + std::to_string(WSAGetLastError()));
	}

	_listenSocketThread = std::thread{ std::bind(&SHttpManagerLowImpl::ThreadListenSocketFunc, this) };
}

//===============================================================================

SHttpManagerLowImpl::~SHttpManagerLowImpl()
{
	// Остановить поток
	_isRequestThreadToFinish = true;
	_listenSocketThread.join();

	// Деинициализировать всё
	if (_listen_socket != INVALID_SOCKET) {
		closesocket(_listen_socket);
	}

	if (_isWsaInited) {
		WSACleanup();
	}
}

//===============================================================================

void SHttpManagerLowImpl::ThreadExitMsg(const std::string& errorMsg)
{
	Utils::MutexLock lock(_fatalErrorInTheradTextMutex);
	if (_fatalErrorInTheradText.empty()) {
		_fatalErrorInTheradText = errorMsg;
	}
}

//===============================================================================

void SHttpManagerLowImpl::ThreadListenSocketFunc()  
{
	std::vector<char> buf;
	buf.resize(HTTP_BUF_SIZE);
	int client_socket = INVALID_SOCKET;

	while (true) {
		// Ждём входящего подключения
		int socketsReadyNum = 0;
		do {
			if (_isRequestThreadToFinish) {
				return;
			}
			fd_set readSet;
			FD_ZERO(&readSet);
			FD_SET(_listen_socket, &readSet);
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 200000;  // Таймаут ожидания в микросекундах. Для того, чтобы при выходе из приложения увидеть флаг запроса завершения потока
			socketsReadyNum = select(_listen_socket, &readSet, NULL, NULL, &timeout);
		} while (socketsReadyNum == 0);
		if (socketsReadyNum == -1) {
			ThreadExitMsg("select failed: " + std::to_string(WSAGetLastError()));
			return;
		}

		// Дождались. Обрабатываем входящее подключение. Поскольку select уже показал, что оно есть, то accept не будет блокирующим
		client_socket = accept(_listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			ThreadExitMsg("accept failed: " + std::to_string(WSAGetLastError()));
			return;
		}

		// Читаем данные из полученного сокета. Сначала заголовок, потом тело 
		int readBytesNum = 0;
		for (int i = 0; i < 2; ++i) { 
			readBytesNum = recv(client_socket, buf.data(), HTTP_BUF_SIZE, 0);

			if (readBytesNum == HTTP_BUF_SIZE) {
				ThreadExitMsg("recv buffer overflow: " + std::to_string(WSAGetLastError()));
				return;
			}
			else if (readBytesNum == SOCKET_ERROR) {
					ThreadExitMsg("recv failed: " + std::to_string(WSAGetLastError()));
					return;
				}
				else if (readBytesNum == 0) {
					// соединение закрыто клиентом
					break;
				}
		}

		if (readBytesNum <= 0) {
			continue;
		}

		DeserializationBuffer deserialBuf((const uint8_t*)buf.data(), readBytesNum);
		SerializationBuffer serialBuf;

		_requestCallback(deserialBuf, serialBuf); // Вызываем коллбек из потока для обработки запроса и формирования ответа

		std::stringstream responseHeader; // Заголовок ответа
		responseHeader << "HTTP/1.1 200 OK\r\n"
			<< "Version: HTTP/1.1\r\n"
			<< "Content-Type: text/html; charset=utf-8\r\n"
			<< "Content-Length: " << serialBuf.GetSize()
			<< "\r\n\r\n";

		std::vector<uint8_t> responseFull; // Здесь склеиваем заголовок и тело
		responseFull.resize(responseHeader.str().length() + serialBuf.GetSize());
		memcpy(responseFull.data(), responseHeader.str().c_str(), responseHeader.str().length());
		memcpy(responseFull.data() + responseHeader.str().length(), serialBuf.GetData(), serialBuf.GetSize());

		// Отправляем ответ клиенту с помощью функции send
		int result = send(client_socket, (const char*)responseFull.data(), responseFull.size(), 0);
		if (result == SOCKET_ERROR) {
			Log("send failed: " + std::to_string(WSAGetLastError()));  // произошла ошибка при отправке данных
		}
		// Закрываем соединение к клиентом
		closesocket(client_socket);
	}
}

//===============================================================================

void SHttpManagerLowImpl::Update(double dt)
{
	// Если поток завершился с ошибкой, то завершить приложение с этой ошибкой
	std::string copyStr;
	{
		Utils::MutexLock lock(_fatalErrorInTheradTextMutex);
		copyStr = _fatalErrorInTheradText;
	}
	if (!copyStr.empty()) {
		ExitMsg(copyStr);
	}
}