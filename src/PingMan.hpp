#pragma once
#include <winsock2.h>
#include <unordered_map>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "Host.hpp"
#include "Status.hpp"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define PING_UPDATE_RATE 1000L
#define MESSAGE_LEN 37

//This is very messy, but it works and that's enough for me
//If someone else wants to fix it up, please do.
namespace PingMan {
	enum {
		ERROR_INITFAILED = -1,
		ERROR_SOCKETFAILED = -2,
		ERROR_SENDTOFAILED = -3,
		ERROR_SELECTFAILED = -4,
		ERROR_PINGTIMEOUT = -5,
		ERROR_RECVFROMFAILED = -6,
		ERROR_INVALIDRESPONSE = -7,
		ERROR_INVALIDIP = -8,

		PING_UNINITIALIZED = -9,

		PACKET_OLLEH = 3
	};

	struct PingInfo {
		long oldTime;
		long ping;
		bool waiting;

		PingInfo() : oldTime(0), ping(PING_UNINITIALIZED), waiting(false) {};
	};

	WSADATA wsa;
	char message[MESSAGE_LEN];
	std::unordered_map<long, PingInfo> pings;
	SOCKET sock = INVALID_SOCKET;

	//Helper functions
	namespace {
		void MessageSetup(char* message, SOCKADDR_IN* addr) {
			memset(message, 0, MESSAGE_LEN);
			message[0] = 1;
			memcpy(message + 1, addr, sizeof(SOCKADDR_IN));
			memcpy(message + 17, addr, sizeof(SOCKADDR_IN));
			message[36] = '\xBC';
		}

		int SockAddrInSetup(SOCKADDR_IN* addr, const char* ipstr, short port) {
			unsigned long ip;
			if ((ip = inet_addr(ipstr)) == INADDR_NONE) {
				printf("[WinSock] Invalid ip string %s.\n", ipstr);
				return ERROR_INVALIDIP;
			}

			memset(addr, 0, sizeof(SOCKADDR_IN));
			addr->sin_family = AF_INET;
			addr->sin_port = htons(port);
			addr->sin_addr.S_un.S_addr = ip;

			return 0;
		}

		int SocketSend(SOCKET s, long ip, short port) {
			SOCKADDR_IN addr;

			addr.sin_family = AF_INET;
			addr.sin_port = port;
			addr.sin_addr.S_un.S_addr = ip;

			MessageSetup(message, &addr);

			if (sendto(sock, message, MESSAGE_LEN, 0, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
				printf("[WinSock] sendto() failed with error code: %d.\n", WSAGetLastError());
				return ERROR_SENDTOFAILED;
			}

			return 0;
		}

		int SocketReceive(SOCKET s, long *ip) {
			char response;
			SOCKADDR_IN addr;
			int addr_len = sizeof(addr);
			if (recvfrom(sock, &response, sizeof(response), NULL, (SOCKADDR*)&addr, &addr_len) == SOCKET_ERROR) {
				printf("[WinSock] recvfrom() failed with error code : %d.\n", WSAGetLastError());
				return ERROR_RECVFROMFAILED;
			}

			if (response == PACKET_OLLEH) {
				*ip = addr.sin_addr.S_un.S_addr;
				return 0;
			}
			else
				return ERROR_INVALIDRESPONSE;
		}
	};

	int Init() {
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			printf("[WinSock] Winsock Init failed with error code: %d.\n", WSAGetLastError());
			return ERROR_INITFAILED;
		}

		if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
			printf("[WinSock] socket() failed with error code: %d.\n", WSAGetLastError());
			return ERROR_SOCKETFAILED;
		}
		return 0;
	}

	void Cleanup() {
		WSACleanup();
	}

	//Credits to cc for the central ping manager idea.
	long Ping(long ip, short port) {
		long newTime = GetTickCount();
		PingInfo& ping = pings[ip];
		if(ping.waiting == false && newTime - ping.oldTime > PING_UPDATE_RATE) { 
			if (SocketSend(sock, ip, port) == 0) {
				ping.waiting = true;
				ping.oldTime = newTime;
			}
		}
		return ping.ping;
	}

	long Ping(Host& host) {
		return Ping(host.netIp, host.netPort);
	}

	int Update(long time) {
		fd_set fds;
		TIMEVAL tv;

		FD_ZERO(&fds);
		FD_SET(sock, &fds);

		tv.tv_sec = 0;
		tv.tv_usec = 0;

		long ip;
		int ret;
		while ((ret = select(sock, &fds, NULL, NULL, &tv)) == 1) {
			if (SocketReceive(sock, &ip) == 0) {
				PingInfo& ping = pings[ip];
				ping.waiting = false;
				ping.ping = time - ping.oldTime;
				ping.oldTime = time;
			}
		}
		if (ret == 0) {
			return ERROR_PINGTIMEOUT;
		}
		else if (ret == SOCKET_ERROR) {
			printf("[WinSock] select() failed with error code: %d.\n", WSAGetLastError());
			return ERROR_SELECTFAILED;
		}

		return 0;
	}
}