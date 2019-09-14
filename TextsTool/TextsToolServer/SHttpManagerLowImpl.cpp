#include "pch.h"

#include <iostream>
#include <sstream>
#include <string>

#include <WS2tcpip.h>

#include "SHttpManagerLowImpl.h"
#include "Utils.h"

// ����������, ����� �������� ����������� � DLL-�����������
// ��� ������ � �������
#pragma comment(lib, "Ws2_32.lib")


const int MAX_CLIENT_BUFFER_SIZE = 1024;  // !!! ��������� ���������� ������ (������������, ������� ����� ��������� ����������. ��� ����������� ��������� � ������ ������ ������, ���� ��������� ������, ��� ��� �����)



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
	WSADATA wsaData; // ��������� ��������� ��� �������� ����������
	// � ���������� Windows Sockets
	// ����� ������������� ���������� ������� ���������
	// (������������ Ws2_32.dll)
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	// ���� ��������� ������ ��������� ����������
	if (result != 0) {
		ExitMsg("WSAStartup failed: " + std::to_string(result));
	}
	_isWsaInited = true;

	// ������ ��� ������������� ��������� ������
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET; // AF_INET ����������, ��� �����
	// �������������� ���� ��� ������ � �������
	hints.ai_socktype = SOCK_STREAM; // ������ ��������� ��� ������
	hints.ai_protocol = IPPROTO_TCP; // ���������� �������� TCP
	hints.ai_flags = AI_PASSIVE; // ����� ����� ��������� �� �����, ����� ��������� �������� ����������

	// �������������� ���������, �������� ����� ������ - addr
	// ��� HTTP-������ ����� ������ �� 8000-� ����� ����������
	result = getaddrinfo("127.0.0.1", "8000", &hints, &_addr);

	// ���� ������������� ��������� ������ ����������� � �������,
	// ������� ���������� �� ���� � �������� ���������� ���������
	if (result != 0) {
		ExitMsg("getaddrinfo failed: " + std::to_string(result));
	}

	// �������� ������
	_listen_socket = socket(_addr->ai_family, _addr->ai_socktype, _addr->ai_protocol);
	// ���� �������� ������ ����������� � �������, ������� ���������,
	// ����������� ������, ���������� ��� ��������� addr,
	// ��������� dll-���������� � ��������� ���������
	if (_listen_socket == INVALID_SOCKET) {
		ExitMsg("Error at socket: " + std::to_string(WSAGetLastError()));
	}

	// ����������� ����� � IP-������
	result = bind(_listen_socket, _addr->ai_addr, (int)_addr->ai_addrlen);

	// ���� ��������� ����� � ������ �� �������, �� ������� ���������
	// �� ������, ����������� ������, ���������� ��� ��������� addr.
	// � ��������� �������� �����.
	// ��������� DLL-���������� �� ������ � ��������� ���������.
	if (result == SOCKET_ERROR) {
		ExitMsg("bind failed with error: " + std::to_string(WSAGetLastError()));
	}

	// �������������� ��������� �����
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
	// ���������� ������


	// ������������������ ��
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


//!!! ����������� ����������� �����, ����� ����� ���� ����� �� ������ ��� ������ ����������� ������. ��������, ����� � ����� ��� ���������� ������ �� �����������. 
// ����� ����� ��������� � Update ����� ����������
// ������ ����� WSACleanup � �������� ������ ���������� accept ������� ���������� � ���������� ������?

void SHttpManagerLowImpl::ThreadListenSocketFunc()  
{
	char buf[MAX_CLIENT_BUFFER_SIZE];
	int client_socket = INVALID_SOCKET;

	while(true) {
		// ��������� �������� ����������
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

		std::stringstream response; // ���� ����� ������������ ����� �������
		std::stringstream response_body; // ���� ������

		if (result == SOCKET_ERROR) {
			// ������ ��������� ������
			cerr << "recv failed: " << result << "\n";
			closesocket(client_socket);
		}
		else if (result == 0) {
			// ���������� ������� ��������
			cerr << "connection closed...\n";
		}
		else if (result > 0) {
			// �� ����� ����������� ������ ���������� ������, ������� ������ ����� ����� ������
			// � ������ �������.
			buf[result] = '\0';

			// ������ ������� ��������
			// ��������� ���� ������ (HTML)
			response_body << "<title>Test C++ HTTP Server</title>\n"
				<< "<h1>MY Test page</h1>\n"
				<< "<p>This is body of the test page...</p>\n"
				<< "<h2>Request headers</h2>\n"
				<< "<pre>" << buf << "</pre>\n"
				<< "<em><small>Test C++ Http Server</small></em>\n";

			// ��������� ���� ����� ������ � �����������
			response << "HTTP/1.1 200 OK\r\n"
				<< "Version: HTTP/1.1\r\n"
				<< "Content-Type: text/html; charset=utf-8\r\n"
				<< "Content-Length: " << response_body.str().length()
				<< "\r\n\r\n"
				<< response_body.str();

			// ���������� ����� ������� � ������� ������� send
			result = send(client_socket, response.str().c_str(),
				response.str().length(), 0);

			if (result == SOCKET_ERROR) {
				// ��������� ������ ��� �������� ������
				cerr << "send failed: " << WSAGetLastError() << "\n";
			}
			// ��������� ���������� � ��������
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