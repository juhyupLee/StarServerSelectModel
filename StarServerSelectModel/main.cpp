#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <list>
#include <WS2tcpip.h>
#include "SocketLog.h"
#include <locale>

#define SERVER_PORT 3000
SOCKET g_ListenSocket;
enum 
{
	ALLOC_ID,
	GEN_STAR,
	DEL_STAR,
	MOVE_STAR
};

struct Session
{
	SOCKET socket;
	int32_t id;
	WCHAR ip[16];
	short port;
	int32_t x;
	int32_t y;
};

std::list<Session*> g_ListSession;
int32_t g_ClientID = 100;

void Accept();
void Network();
void Render();
void PacketProcess(char* buffer,int recvLen,Session* session);
void Disconnect(Session* inSession);
void BroadcastSend(char* buffer, int len, Session* exceptSession);
void InitializeListen();
void UnicastSend(char* buffer, int len, Session* toSession);
int main()
{
	InitializeListen();
	while (true)
	{
		Network();
		Render();
	}
}

void Accept()
{
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	SOCKET clientSocket = accept(g_ListenSocket, (sockaddr*)&clientAddr, &addrLen);

	Session* newSession = new Session();

	// id, IP,Port, socket저장
	newSession->id = g_ClientID++;
	InetNtopW(AF_INET, &clientAddr.sin_addr.S_un.S_addr, newSession->ip, 16);
	newSession->port = ntohs(clientAddr.sin_port);
	newSession->socket = clientSocket;

	// 초기 위치
	newSession->x = 50;
	newSession->y = 10;
	g_ListSession.push_back(newSession);

	wprintf(L"connect[IP:%s] [PORT:%d]\n", newSession->ip, newSession->port);

	//-----------------------------------------------
	// 클라가 들어오면 일단 그 클라에게 ID를 할당하고, 자기 별생성을 포함하여, 접속한 유저들의 별생성을 지시
	//-----------------------------------------------
	int packet[4];
	packet[0] = ALLOC_ID;
	packet[1] = newSession->id;
	UnicastSend((char*)&packet, sizeof(packet), newSession);

	for (auto session : g_ListSession)
	{
		int packet[4];
		packet[0] =  GEN_STAR;
		packet[1] = session->id;
		packet[2] = session->x;
		packet[3] = session->y;
		// 여기서 브로드캐스팅을 쓰면 안되는게, 지금 새로 생성된 클라에게만 쏘는것이라서 반복문을 돌면서쏴야된다
		UnicastSend((char*)&packet, sizeof(packet), newSession);
	}
	//-----------------------------------------------
	// 기존 클라에게, 새로운  클라가 들어왔다고 별생성을 지시
	//-----------------------------------------------
	packet[0] = GEN_STAR;
	packet[1] = newSession->id;
	packet[2] = newSession->x;
	packet[3] = newSession->y;
	BroadcastSend((char*)&packet,sizeof(packet),newSession);

}

void Network()
{
	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(g_ListenSocket, &readSet);
	char buffer[64]; // 16*4

	//------------------------------------
	// readSet 세팅
	//------------------------------------
	for (auto session : g_ListSession)
	{
		FD_SET(session->socket, &readSet);
	}

	//------------------------------------
	// 어차피 입력이 없으면 Render도 안되니, timeout을 infinite로 해도됨.
	//------------------------------------
	int selectRtn = select(0, &readSet, NULL, NULL, NULL);
	if (selectRtn < 0)
	{
		ERROR_LOG(L"select() error");
		return;
	}

	if (FD_ISSET(g_ListenSocket, &readSet))
	{
		Accept();
	}
	//------------------------------------
	// select로 recv를 감지하고, listen socket 아닐 때 처리.
	//------------------------------------
	for (auto session : g_ListSession)
	{
		if (FD_ISSET(session->socket, &readSet))
		{
			int recvRtn = recv(session->socket, buffer, sizeof(buffer), 0);

			if (recvRtn <= 0)
			{
				ERROR_LOG(L"recv() Error");
				Disconnect(session);
			}
			PacketProcess(buffer,recvRtn, session);
			
		}
	}
	
}

void Render()
{

}

void PacketProcess(char* buffer, int recvLen,Session* session)
{
	if (recvLen < 16)
	{
		ERROR_LOG(L"Recv  Divdide Error");
	}
	char* tempBuffer;
	for (int i = 0; i <= recvLen-16; i++)
	{
		tempBuffer = buffer + i;
		int* packet = (int*)tempBuffer;

		switch (*packet)
		{
		case MOVE_STAR:
		{
			// 보내온, x,y가 범위에 맞는지 체크해야함


			// Move Star를 보내온 클라 외에 전부 BroadCast Send
			int arr[4];
			arr[0] = MOVE_STAR;
			arr[1] = *(packet + 1);
			arr[2] = *(packet + 2);
			arr[3] = *(packet + 3);
			BroadcastSend((char*)arr,sizeof(arr), session);
			break;
		}
		default:
			// 정해진 프로토콜 이외의것을 보내면, 잘못된클라이기때문에 Disconnect
			Disconnect(session);
			break;
		}
	}
}

void Disconnect(Session* inSession)
{
}

void BroadcastSend(char* buffer,int len, Session* exceptSession)
{
	if (!exceptSession)
	{
		for (auto session : g_ListSession)
		{
			int sendRtn = send(session->socket, buffer, len, 0);
			if (sendRtn < 0)
			{
				ERROR_LOG(L"send() error");
				Disconnect(session);
			}
		}
	}
	else
	{
		for (auto session : g_ListSession)
		{
			if (session == exceptSession)
			{
				continue;
			}
			int sendRtn = send(session->socket, buffer, sizeof(buffer), 0);
			if (sendRtn < 0)
			{
				ERROR_LOG(L"send() error");
				Disconnect(session);
			}
		}
	}
}

void InitializeListen()
{
	WSAData wsaData;
	SOCKADDR_IN serverAddr;
	setlocale(LC_ALL, "Korean");

	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		ERROR_LOG(L"WSAStartup Error");
		return ;
	}

	g_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (g_ListenSocket == INVALID_SOCKET)
	{
		ERROR_LOG(L"socket() Error");
		return ;
	}
	//------------------------------------------------
	// 주소 바인딩
	//------------------------------------------------
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (0 != bind(g_ListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)))
	{
		ERROR_LOG(L"bind() error");
		return;
	}
	
	listen(g_ListenSocket, SOMAXCONN);
	wprintf(L"Server Listening....\n");

}

void UnicastSend(char* buffer,int len, Session* toSession)
{
	if (toSession == nullptr)
	{
		return;
	}
	
	int sendRtn = send(toSession->socket, buffer, len, 0);
	if (sendRtn < 0)
	{
		ERROR_LOG(L"send Error()");
		Disconnect(toSession);
	}
}

