#include <iostream>
#include <sstream>
#include <string>

#include <WS2tcpip.h>

#include "SHttpManagerLowImpl.h"
#include "Utils.h"
#include "../SharedSrc/Shared.h"

// ����������, ����� �������� ����������� � DLL-�����������
// ��� ������ � �������
#pragma comment(lib, "Ws2_32.lib")


// !!! ��� ����������� ��������� � ������ ������ ������, ���� ��������� ������, ��� ��� �����

void ExitMsg(const std::string& message);

//===============================================================================

SHttpManagerLowImpl::SHttpManagerLowImpl(std::function<void(DeserializationBuffer&, SerializationBuffer&)> requestCallback) :
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

	struct addrinfo* _addr = nullptr; // ���������, �������� ���������� �� IP-������  ���������� ������
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
		freeaddrinfo(_addr);
		ExitMsg("Error at socket: " + std::to_string(WSAGetLastError()));
	}

	// ����������� ����� � IP-������
	result = bind(_listen_socket, _addr->ai_addr, (int)_addr->ai_addrlen);
	freeaddrinfo(_addr);

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

SHttpManagerLowImpl::~SHttpManagerLowImpl()
{
	// ���������� �����
	_isRequestThreadToFinish = true;
	_listenSocketThread.join();

	// ������������������ ��
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
		// ��� ��������� �����������
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
			timeout.tv_usec = 200000;  // ������� �������� � �������������. ��� ����, ����� ��� ������ �� ���������� ������� ���� ������� ���������� ������
			socketsReadyNum = select(_listen_socket, &readSet, NULL, NULL, &timeout);
		} while (socketsReadyNum == 0);
		if (socketsReadyNum == -1) {
			ThreadExitMsg("select failed: " + std::to_string(WSAGetLastError()));
			return;
		}

		// ���������. ������������ �������� �����������. ��������� select ��� �������, ��� ��� ����, �� accept �� ����� �����������
		client_socket = accept(_listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			ThreadExitMsg("accept failed: " + std::to_string(WSAGetLastError()));
			return;
		}

		// ������ ������ �� ����������� ������. ������� ���������, ����� ���� 
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
					// ���������� ������� ��������
					break;
				}
		}

		if (readBytesNum <= 0) {
			continue;
		}

		DeserializationBuffer deserialBuf((const uint8_t*)buf.data(), readBytesNum);
		SerializationBuffer serialBuf;

		_requestCallback(deserialBuf, serialBuf); // �������� ������� �� ������ ��� ��������� ������� � ������������ ������

		std::stringstream responseHeader; // ��������� ������
		responseHeader << "HTTP/1.1 200 OK\r\n"
			<< "Version: HTTP/1.1\r\n"
			<< "Content-Type: text/html; charset=utf-8\r\n"
			<< "Content-Length: " << serialBuf.GetSize()
			<< "\r\n\r\n";

		std::vector<uint8_t> responseFull; // ����� ��������� ��������� � ����
		responseFull.resize(responseHeader.str().length() + serialBuf.GetSize());
		memcpy(responseFull.data(), responseHeader.str().c_str(), responseHeader.str().length());
		memcpy(responseFull.data() + responseHeader.str().length(), serialBuf.GetData(), serialBuf.GetSize());

		// ���������� ����� ������� � ������� ������� send
		int result = send(client_socket, (const char*)responseFull.data(), responseFull.size(), 0);
		if (result == SOCKET_ERROR) {
			Log("send failed: " + std::to_string(WSAGetLastError()));  // ��������� ������ ��� �������� ������
		}
		// ��������� ���������� � ��������
		closesocket(client_socket);
	}
}

//===============================================================================

void SHttpManagerLowImpl::Update(double dt)
{
	// ���� ����� ���������� � �������, �� ��������� ���������� � ���� �������
	std::string copyStr;
	{
		Utils::MutexLock lock(_fatalErrorInTheradTextMutex);
		copyStr = _fatalErrorInTheradText;
	}
	if (!copyStr.empty()) {
		ExitMsg(copyStr);
	}
}