#include "pch.h"

#include <iostream>
#include <sstream>
#include <string>

#include <WS2tcpip.h>

#include "SHttpManagerLowImpl.h"

// ����������, ����� �������� ����������� � DLL-�����������
// ��� ������ � �������
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
	hints.ai_flags = AI_PASSIVE; // ����� ����� ��������� �� �����,
	// ����� ��������� �������� ����������

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
