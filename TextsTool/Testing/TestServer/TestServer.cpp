/*
result = getaddrinfo("127.0.0.1", "8000", &hints, &addr);
freeaddrinfo(addr);

int listen_socket = socket(addr->ai_family, addr->ai_socktype,
closesocket(listen_socket);

int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
WSACleanup();

*/

#include "pch.h"
#include <iostream>
#include <sstream>
#include <string>

#include <WinSock2.h>
#include <WS2tcpip.h>

// Необходимо, чтобы линковка происходила с DLL-библиотекой
// Для работы с сокетам
#pragma comment(lib, "Ws2_32.lib")

using std::cerr;

int main()
{
	std::string bigString;
	for (int i = 0; i < 5000; ++i) {
		bigString += "02345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
	}
	int sum = 0;
	for (int i = 0; i < bigString.size(); ++i) {
		bigString[i] = rand();
		sum += (unsigned char)bigString[i];
	}
	printf("sum = %d\n", sum);

	WSADATA wsaData; // служебная структура для хранение информации
	// о реализации Windows Sockets
	// старт использования библиотеки сокетов процессом
	// (подгружается Ws2_32.dll)
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	// Если произошла ошибка подгрузки библиотеки
	if (result != 0) {
		cerr << "WSAStartup failed: " << result << "\n";
		return result;
	}

	struct addrinfo* addr = NULL; // структура, хранящая информацию
	// об IP-адресе  слущающего сокета

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
	result = getaddrinfo("127.0.0.1", "8000", &hints, &addr);

	// Если инициализация структуры адреса завершилась с ошибкой,
	// выведем сообщением об этом и завершим выполнение программы
	if (result != 0) {
		cerr << "getaddrinfo failed: " << result << "\n";
		WSACleanup(); // выгрузка библиотеки Ws2_32.dll
		return 1;
	}

	// Создание сокета
	int listen_socket = socket(addr->ai_family, addr->ai_socktype,
		addr->ai_protocol);
	// Если создание сокета завершилось с ошибкой, выводим сообщение,
	// освобождаем память, выделенную под структуру addr,
	// выгружаем dll-библиотеку и закрываем программу
	if (listen_socket == INVALID_SOCKET) {
		cerr << "Error at socket: " << WSAGetLastError() << "\n";
		freeaddrinfo(addr);
		WSACleanup();
		return 1;
	}

	// Привязываем сокет к IP-адресу
	result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);
	freeaddrinfo(addr);

	// Если привязать адрес к сокету не удалось, то выводим сообщение
	// об ошибке, освобождаем память, выделенную под структуру addr.
	// и закрываем открытый сокет.
	// Выгружаем DLL-библиотеку из памяти и закрываем программу.
	if (result == SOCKET_ERROR) {
		cerr << "bind failed with error: " << WSAGetLastError() << "\n";
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	// Инициализируем слушающий сокет
	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "listen failed with error: " << WSAGetLastError() << "\n";
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}


	const int max_client_buffer_size = 1024;
	char buf[max_client_buffer_size];
	int client_socket = INVALID_SOCKET;

	while(true) {
		// Ждём выходящего подключения
		int socketsReadyNum = 0;
		do {
			// See if connection pending
			fd_set readSet;
			FD_ZERO(&readSet);
			FD_SET(listen_socket, &readSet);
			timeval timeout;
			timeout.tv_sec = 0;  // Zero timeout (poll)
			timeout.tv_usec = 200000;

			socketsReadyNum = select(listen_socket, &readSet, NULL, NULL, &timeout);
		} while (socketsReadyNum == 0);
		printf("\nread select. socketsReadyNum = %d \n", socketsReadyNum);

		// Обрабатываем входящее подключение
		client_socket = accept(listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			cerr << "accept failed: " << WSAGetLastError() << "\n";
			closesocket(listen_socket);
			WSACleanup();
			return 1;
		}

		int readBytesNum = 0;
		for (int i = 0; i < 2; ++i) {  // Первый раз получаем заголовок, второй раз получаем тело запроса
			readBytesNum = recv(client_socket, buf, max_client_buffer_size, 0);

			if (readBytesNum == SOCKET_ERROR) {
				// ошибка получения данных
				cerr << "recv failed: " << readBytesNum << "\n";
				break;
			}
			else if (readBytesNum == 0) {
				// соединение закрыто клиентом
				cerr << "connection closed...\n";
				break;
			}
			else if (readBytesNum > 0) {
				printf("result = %d\n", readBytesNum);
				buf[readBytesNum] = '\0';
				puts(buf);
				if (i > 0) {
					for (int i2 = 0; i2 < readBytesNum; ++i2) {
						printf("%d ", buf[i2]);
					}
				}
				puts("\n");
			}
		}

		if (readBytesNum > 0) {
			std::stringstream response; // сюда будет записываться ответ клиенту
			//std::stringstream response_body; // тело ответа
			// Данные успешно получены
			// формируем тело ответа (HTML)
			//response_body << "<title>Test C++ HTTP Server</title>\n"
			//	<< "<h1>MY Test page</h1>\n"
			//	<< "<p>This is body of the test page...</p>\n"
			//	<< "<h2>Request headers</h2>\n"
			//	//			<< "<pre>" << buf << "</pre>\n"
			//	<< "<em><small>Test C++ Http Server</small></em>\n";
			//// Формируем весь ответ вместе с заголовками
			//response << "HTTP/1.1 200 OK\r\n"
			//	<< "Version: HTTP/1.1\r\n"
			//	<< "Content-Type: text/html; charset=utf-8\r\n"
			//	<< "Content-Length: " << response_body.str().length()
			//	<< "\r\n\r\n"
			//	<< response_body.str();

			// Формируем весь ответ вместе с заголовками
			response << "HTTP/1.1 200 OK\r\n"
				<< "Version: HTTP/1.1\r\n"
				<< "Content-Type: text/html; charset=utf-8\r\n"
				<< "Content-Length: " << bigString.size()
				<< "\r\n\r\n"
				<< bigString;

			// Отправляем ответ клиенту с помощью функции send
			//unsigned char arr[10] = {48, 49, 50, 51};
			//result = send(client_socket, (const char*)&arr[0], 4, 0);

			result = send(client_socket, response.str().c_str(), response.str().length(), 0);

			if (result == SOCKET_ERROR) {
				// произошла ошибка при отправле данных
				cerr << "send failed: " << WSAGetLastError() << "\n";
			}
			// Закрываем соединение с клиентом
		}
		closesocket(client_socket);
	}

	// Убираем за собой
	closesocket(listen_socket);
	WSACleanup();
	return 0;
}
