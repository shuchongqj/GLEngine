#include "Network/TCPSocket.h"

#include "Utils/StringUtils.h"

#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>

TCPSocket::~TCPSocket()
{
	disconnect();
}

void TCPSocket::listen(Address a_ip, Port a_port)
{
	assert(a_ip && a_port);
	assert(m_socket == 0);

	eastl::string portString = StringUtils::to_string(a_port);
	eastl::string ipString = a_ip;
	int result = 0;
	
	ADDRINFOA hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	PADDRINFOA info = {};
	::getaddrinfo(ipString.c_str(), portString.c_str(), &hints, &info);

	SOCKET listenSocket = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		print("Socket creation failed: %i\n", WSAGetLastError());
		::freeaddrinfo(info);
		assert(false);
		return;
	}

	result = ::bind(listenSocket, info->ai_addr, int(info->ai_addrlen));
	if (result == SOCKET_ERROR) {
		print("Socket bind failed: %i\n", WSAGetLastError());
		::freeaddrinfo(info);
		closesocket(listenSocket);
		return;
	}
	::freeaddrinfo(info);
	
	result = ::listen(listenSocket, SOMAXCONN_HINT(1));
	if (result == SOCKET_ERROR) 
	{
		print("Socket listen failed: %i\n", WSAGetLastError());
		::closesocket(listenSocket);
		return;
	}

	SOCKET connectedSocket = ::accept(listenSocket, NULL, NULL);
	if (connectedSocket == INVALID_SOCKET) 
	{
		print("Socket accept failed: %i\n", WSAGetLastError());
		::closesocket(connectedSocket);
		return;
	}

	print("Socket connection opened: %i\n", connectedSocket);

	result = ::closesocket(listenSocket);
	if (result == SOCKET_ERROR)
	{
		print("Socket close failed: %i\n", WSAGetLastError());
	}
	m_socket = connectedSocket;
}

void TCPSocket::connect(Address a_ip, Port a_port)
{
	assert(a_ip && a_port);
	assert(m_socket == 0);

	eastl::string portString = StringUtils::to_string(a_port);
	eastl::string ipString = a_ip;

	ADDRINFOA hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	PADDRINFOA info = {};
	::getaddrinfo(ipString.c_str(), portString.c_str(), &hints, &info);

	SOCKET tcpSocket = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (tcpSocket == INVALID_SOCKET)
	{
		print("Socket creation failed: %i\n", WSAGetLastError());
		::freeaddrinfo(info);
		assert(false);
		return;
	}
	m_socket = tcpSocket;

	int result = ::connect(m_socket, info->ai_addr, int(info->ai_addrlen));
	::freeaddrinfo(info);
	if (result == SOCKET_ERROR) 
	{
		print("Socket connect failed: %i\n", WSAGetLastError());
		disconnect();
		return;
	}

	print("Socket connected: %i\n", m_socket);
}

void TCPSocket::disconnect()
{
	if (m_socket)
	{
		int result = ::shutdown(m_socket, SD_SEND);
		if (result == SOCKET_ERROR) 
			print("Socket shutdown failed: %i\n", WSAGetLastError());
		result = ::closesocket(m_socket);
		if (result == SOCKET_ERROR)
			print("Socket close failed: %i\n", WSAGetLastError());
		else
			print("Socket connection closed: %i\n", m_socket);
		m_socket = 0;
	}
}

void TCPSocket::send(gsl::span<const byte> a_data)
{
	assert(m_socket);
	assert(a_data.length_bytes());
	int result = ::send(m_socket, rcast<const char*>(a_data.data()), int(a_data.length_bytes()), 0);
	if (result == SOCKET_ERROR)
	{
		print("Socket send failed: %i\n", result);
		disconnect();
		return;
	}
}

bool TCPSocket::receive()
{
	if (!m_socket)
		return false;
#if USE_COPY_BUFFER
	char buf[BUFFER_SIZE / 2];
	size_t maxReceivable = eastl::min(m_ringQueue.capacity() - m_ringQueue.size(), ARRAY_SIZE(buf));
#else
	char* buf = rcast<char*>(m_ringQueue.getTail());
	size_t maxReceivable = m_ringQueue.getNumEmptyFromTailToEnd();
#endif
	if (maxReceivable > 0)
	{
		int result = ::recv(m_socket, buf, (int) maxReceivable, 0);
		if (result > 0)
		{
#if USE_COPY_BUFFER
			bool r = m_ringQueue.push_back(buf, result);
			assert(r);
#else
			m_ringQueue.appendToTail(result);
#endif
			return true;
		}
		else
		{
			if (result < 0)
				print("Socket receive failed, err: %i\n", result);
			disconnect();
			return false;
		}
	}
	else
	{
		print("Socket receive failed because queue is full!\n");
		return false;
	}
}

bool TCPSocket::read(span<byte> data)
{
	return m_ringQueue.pop_front(data.data(), data.length_bytes());
}