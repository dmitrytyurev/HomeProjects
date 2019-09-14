#include "pch.h"

#include <iostream>
#include <sstream>
#include <string>

#include <WS2tcpip.h>

#include "SHttpManagerLowImpl.h"
#include "Utils.h"

// Необходимо, чтобы линковка происходила с DLL-библиотекой
// Для работы с сокетам
#pragma comment(lib, "Ws2_32.lib")


const int MAX_CLIENT_BUFFER_SIZE = 1024;  // !!! Настроить правильную длинну (максимальную, которую сокет позволяет пересылать. При конвертации сообщения в пакеты должны резать, если сообщение больше, чем это число)



void ExitMsg(const std::string& message);

//===============================================================================
//
//===============================================================================

SHttpManagerLowImpl::SHttpManagerLowImpl(std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> requestCallback) :
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
		ExitMsg("Error at socket: " + std::to_string(WSAGetLastError()));
	}

	// Привязываем сокет к IP-адресу
	result = bind(_listen_socket, _addr->ai_addr, (int)_addr->ai_addrlen);

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
//
//===============================================================================

SHttpManagerLowImpl::~SHttpManagerLowImpl()
{
	// Остановить потоки


	// Деинициализировать всё
	if (_listen_socket != INVALID_SOCKET) {
		closesocket(_listen_socket);
	}
	if (_addr) {
		freeaddrinfo(_addr);
	}

	if (_isWsaInited) {
		WSACleanup();
	}
}

//===============================================================================
//
//===============================================================================


//!!! Попробовать асинхронный сокет, чтобы можно было выйти из потока при вызове деструктора класса. Вероятно, тогда и поток для слушающего сокета не потребуется. 
// Можно будет проверять в Update новые соединения
// Мождет вызов WSACleanup в основном потоке заставляет accept вернуть управление в неосновном потоке?

void SHttpManagerLowImpl::ThreadListenSocketFunc()  
{
	char buf[MAX_CLIENT_BUFFER_SIZE];
	int client_socket = INVALID_SOCKET;

	while(true) {
		// Принимаем входящие соединения
		client_socket = accept(_listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			{
				MutexLock lock(_fatalErrorInTheradTextMutex);
				if (_fatalErrorInTheradText.empty()) {
					_fatalErrorInTheradText = "accept failed: " + std::to_string(WSAGetLastError());
				}
			}
			return;
		}

		int result = recv(client_socket, buf, MAX_CLIENT_BUFFER_SIZE, 0);

		std::stringstream response; // сюда будет записываться ответ клиенту
		std::stringstream response_body; // тело ответа

		if (result == SOCKET_ERROR) {
			// ошибка получения данных
			cerr << "recv failed: " << result << "\n";
			closesocket(client_socket);
		}
		else if (result == 0) {
			// соединение закрыто клиентом
			cerr << "connection closed...\n";
		}
		else if (result > 0) {
			// Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки
			// В буфере запроса.
			buf[result] = '\0';

			// Данные успешно получены
			// формируем тело ответа (HTML)
			response_body << "<title>Test C++ HTTP Server</title>\n"
				<< "<h1>MY Test page</h1>\n"
				<< "<p>This is body of the test page...</p>\n"
				<< "<h2>Request headers</h2>\n"
				<< "<pre>" << buf << "</pre>\n"
				<< "<em><small>Test C++ Http Server</small></em>\n";

			// Формируем весь ответ вместе с заголовками
			response << "HTTP/1.1 200 OK\r\n"
				<< "Version: HTTP/1.1\r\n"
				<< "Content-Type: text/html; charset=utf-8\r\n"
				<< "Content-Length: " << response_body.str().length()
				<< "\r\n\r\n"
				<< response_body.str();

			// Отправляем ответ клиенту с помощью функции send
			result = send(client_socket, response.str().c_str(),
				response.str().length(), 0);

			if (result == SOCKET_ERROR) {
				// произошла ошибка при отправле данных
				cerr << "send failed: " << WSAGetLastError() << "\n";
			}
			// Закрываем соединение к клиентом
			closesocket(client_socket);
		}
	}
}

//===============================================================================
//
//===============================================================================

void SHttpManagerLowImpl::Update(double dt)
{
	std::string copyStr;
	{
		MutexLock lock(_fatalErrorInTheradTextMutex);
		copyStr = _fatalErrorInTheradText;
	}

	if (!copyStr.empty()) {
		ExitMsg(copyStr);
	}
}