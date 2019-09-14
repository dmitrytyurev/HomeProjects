#include "pch.h"

#include <iostream>
#include <sstream>
#include <string>

#include <WS2tcpip.h>

#include "SHttpManagerLowImpl.h"

// Необходимо, чтобы линковка происходила с DLL-библиотекой
// Для работы с сокетам
#pragma comment(lib, "Ws2_32.lib")

void ExitMsg(const std::string& message);

//===============================================================================
//
//===============================================================================

SHttpManagerLowImpl::SHttpManagerLowImpl(std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> requestCallback) :
	_requestCallback(requestCallback)
{
}

//===============================================================================
//
//===============================================================================

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
	hints.ai_flags = AI_PASSIVE; // Сокет будет биндиться на адрес,
	// чтобы принимать входящие соединения

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
