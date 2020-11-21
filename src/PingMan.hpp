#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

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

	WSADATA wsa;

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
				printf("[WinSock] Invalid ip string %s.", ipstr);
				return ERROR_INVALIDIP;
			}

			memset(addr, 0, sizeof(SOCKADDR_IN));
			addr->sin_family = AF_INET;
			addr->sin_port = htons(port);
			addr->sin_addr.S_un.S_addr = ip;

			return 0;
		}

		int SocketReceive(SOCKET s, long timout_s = 0, long timeout_us = 0) {
			static fd_set fds;
			static TIMEVAL tv;

			FD_ZERO(&fds);
			FD_SET(s, &fds);

			tv.tv_sec = timout_s;
			tv.tv_usec = timeout_us;

			int ret;
			if ((ret = select(s, &fds, NULL, NULL, &tv)) == 0) {
				return ERROR_PINGTIMEOUT;
			}
			else if (ret == SOCKET_ERROR) {
				printf("[WinSock] select() failed with error code: %d\n", WSAGetLastError());
				return ERROR_SELECTFAILED;
			}

			char response;
			if (recvfrom(s, &response, sizeof(response), NULL, NULL, NULL) == SOCKET_ERROR) {
				printf("[WinSock] recvfrom() failed with error code : %d", WSAGetLastError());
				return ERROR_RECVFROMFAILED;
			}

			return response;
		}
	};

	int Init() {
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			printf("[WinSock] Winsock Init failed with error code: %d", WSAGetLastError());
			return ERROR_INITFAILED;
		}
		return 0;
	}

	void Cleanup() {
		WSACleanup();
	}

	long PingSync(const char* ipstr, short port) {
		SOCKADDR_IN addr;
		SOCKET s = INVALID_SOCKET;
		char response = 0;
		char message[MESSAGE_LEN];

		SockAddrInSetup(&addr, ipstr, port);

		if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
			printf("[WinSock] socket() failed with error code: %d", WSAGetLastError());
			return ERROR_SOCKETFAILED;
		}

		MessageSetup(message, &addr);

		long startTime = GetTickCount();

		if (sendto(s, message, MESSAGE_LEN, 0, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
			printf("[WinSock] sendto() failed with error code: %d", WSAGetLastError());
			closesocket(s);
			return ERROR_SENDTOFAILED;
		}

		int res = SocketReceive(s, 1);
		if (res < 0) {
			closesocket(s);
			return res;
		}

		long endTime = GetTickCount();

		closesocket(s);

		return endTime - startTime;
	}

	class PingAsync {
		SOCKET s;
		bool waiting;
		int error;

		SOCKADDR_IN addr;

		long startTime;
		long lastPing;

		long lastTime;

		char message[MESSAGE_LEN];

	public:
		PingAsync(const char* ipstr, short port) {
			error = 0;
			waiting = false;

			lastPing = PING_UNINITIALIZED;
			startTime = 0;
			lastTime = 0;

			SockAddrInSetup(&addr, ipstr, port);

			if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
				printf("[WinSock] socket() failed with error code: %d", WSAGetLastError());
				error = ERROR_SOCKETFAILED;
				return;
			}

			MessageSetup(message, &addr);
		}

		long Ping() {
			if (!error) {
				if (!waiting) {
					if (sendto(s, message, MESSAGE_LEN, 0, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
						printf("[WinSock] sendto() failed with error code: %d", WSAGetLastError());
						return ERROR_SENDTOFAILED;
					}

					startTime = GetTickCount();
					waiting = true;
				}

				int res = SocketReceive(s);
				if (res < 0) {
					if(res != ERROR_PINGTIMEOUT) waiting = false;
					return res;
				}

				waiting = false;
				lastPing = GetTickCount() - startTime;
				return lastPing;
			}
			else return error;
		}

		//TODO: error handling if neccessary
		void Update(long time) {
			if (!waiting) {
				if (time - lastTime > PING_UPDATE_RATE) {
					Ping();
					lastTime = time;
				}
			}
			else Ping();
		}

		long GetPing() {
			return lastPing;
		}

		~PingAsync() {
			closesocket(s);
		}
	};
}