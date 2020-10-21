#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <list>
#include <WS2tcpip.h>
#include "SocketLog.h"
#include <locale>
#include <process.h>
#include "Console.h"

#define MAX_WIDTH 80
#define MAX_HEIGHT 23
#define PACKET_SIZE 16
#define RECV_LEN PACKET_SIZE*90

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
int g_FPS = 0;

void Accept();
void Network();
void Render();
void PacketProcess(char* buffer,int recvLen,Session* session);
void Disconnect(Session* inSession);
void BroadcastSend(char* buffer, int len, Session* exceptSession);
void InitializeListen();
void UnicastSend(char* buffer, int len, Session* toSession);
void ClearSession();

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
HWND g_hWnd;

WNDCLASS WndClass;

WCHAR g_TextoutBuffer1[256];
WCHAR g_TextoutBuffer2[256];



unsigned int WINAPI TestWindow(LPVOID lpParam);
int main()
{
	InitializeListen();
	//long long fpsTime = GetTickCount64();
	HANDLE windowThread;
	windowThread = (HANDLE)_beginthreadex(NULL, 0, TestWindow, NULL, 0, NULL);

	while (true)
	{

		Network();
		Render();
	}
}
unsigned int WINAPI TestWindow(LPVOID lpParam)
{
	HINSTANCE hInstance;
	HWND hWnd;
	MSG Message;

	hInstance = GetModuleHandle(NULL);
	g_hInst = hInstance;

	//������ Ŭ���� �ʱ�ȭ
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.lpszClassName = L"ApiBase";
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;

	////������ Ŭ���� ����.
	RegisterClass(&WndClass);

	//������ ��ü ����.
	hWnd = CreateWindow(L"ApiBase",
		L"Test",
		WS_OVERLAPPEDWINDOW,
		10,// X
		100,// Y
		400,// Width
		400,// Height
		NULL,
		(HMENU)NULL,
		hInstance,
		NULL);

	//������ â ����.
	ShowWindow(hWnd, 1);

	g_hWnd = hWnd;


	while (GetMessage(&Message, 0, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return 0;
}

void Accept()
{
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	SOCKET clientSocket = accept(g_ListenSocket, (sockaddr*)&clientAddr, &addrLen);

	Session* newSession = new Session();

	// id, IP,Port, socket����
	newSession->id = g_ClientID++;
	InetNtopW(AF_INET, &clientAddr.sin_addr.S_un.S_addr, newSession->ip, 16);
	newSession->port = ntohs(clientAddr.sin_port);
	newSession->socket = clientSocket;

	// �ʱ� ��ġ
	newSession->x = 50;
	newSession->y = 10;
	g_ListSession.push_back(newSession);

	wprintf(L"connect[IP:%s] [PORT:%d]\n", newSession->ip, newSession->port);

	//-----------------------------------------------
	// Ŭ�� ������ �ϴ� �� Ŭ�󿡰� ID�� �Ҵ��ϰ�, �ڱ� �������� �����Ͽ�, ������ �������� �������� ����
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
		// ���⼭ ��ε�ĳ������ ���� �ȵǴ°�, ���� ���� ������ Ŭ�󿡰Ը� ��°��̶� �ݺ����� ���鼭���ߵȴ�
		UnicastSend((char*)&packet, sizeof(packet), newSession);
	}
	//-----------------------------------------------
	// ���� Ŭ�󿡰�, ���ο�  Ŭ�� ���Դٰ� �������� ����
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
	char buffer[RECV_LEN]; 

	//------------------------------------
	// readSet ����
	//------------------------------------
	for (auto session : g_ListSession)
	{
		FD_SET(session->socket, &readSet);
	}

	//------------------------------------
	// ������ �Է��� ������ Render�� �ȵǴ�, timeout�� infinite�� �ص���.
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
	// select�� recv�� �����ϰ�, listen socket �ƴ� �� ó��.
	//------------------------------------
	for (auto session : g_ListSession)
	{
		if (FD_ISSET(session->socket, &readSet))
		{
			int recvRtn = recv(session->socket, buffer, sizeof(buffer), 0);

			//For Debug
			wsprintf(g_TextoutBuffer1,L"Recv Return:%d", recvRtn);
			InvalidateRect(g_hWnd, NULL, TRUE);

			if (recvRtn <= 0)
			{
				ERROR_LOG(L"recv() Error");
				Disconnect(session);
			}
			else
			{
				PacketProcess(buffer, recvRtn, session);
			}
		}
	}
	
}

void Render()
{
	ClearSession();
	CConsole::GetInstance()->Buffer_Clear();

	for (auto session : g_ListSession)
	{
		if (session->x < 0 || session->y < 0)
		{
			continue;
		}
		if (session->x > MAX_WIDTH || session->y >MAX_HEIGHT)
		{
			continue;
		}
		CConsole::GetInstance()->Sprite_Draw(session->x, session->y, '*');
	}
	CConsole::GetInstance()->Buffer_Flip();
}

void PacketProcess(char* buffer, int recvLen,Session* session)
{
	if (recvLen < PACKET_SIZE)
	{
		ERROR_LOG(L"Recv  Divdide Error");
	}
	char* tempBuffer;
	for (int i = 0; i <= recvLen- PACKET_SIZE; i+= PACKET_SIZE)
	{
		tempBuffer = buffer + i;
		int* packet = (int*)tempBuffer;

		switch (*packet)
		{
		case MOVE_STAR:
		{
			// ������ ������ ��ǥ�� �������־����.
			session->x = *(packet + 2);
			session->y = *(packet + 3);

			// Move Star�� ������ Ŭ�� �ܿ� ���� BroadCast Send
			int arr[4];
			arr[0] = MOVE_STAR;
			arr[1] = *(packet + 1);
			arr[2] = *(packet + 2);
			arr[3] = *(packet + 3);
			BroadcastSend((char*)arr,sizeof(arr), session);
			break;
		}
		default:
			// ������ �������� �̿��ǰ��� ������, �߸���Ŭ���̱⶧���� Disconnect
			Disconnect(session);
			break;
		}
	}
}

void Disconnect(Session* inSession)
{
	//-------------------------------------------
	// ���� �ɼ����� ����. 
	//-------------------------------------------
	LINGER optVal;
	optVal.l_onoff = 1;
	optVal.l_linger = 0;

	int optionRtn = setsockopt(inSession->socket, SOL_SOCKET, SO_LINGER, (char*)&optVal, sizeof(optVal));
	if (optionRtn == SOCKET_ERROR)
	{
		ERROR_LOG(L"lingerOption Error()");
	}

	closesocket(inSession->socket);

	//-------------------------------------------
	// �ٸ� ���ӵ��ִ� Ŭ�󿡰�, �� Ŭ�� ������ٰ� �˷�����.
	//-------------------------------------------

	int packet[4];
	packet[0] = DEL_STAR;
	packet[1] = inSession->id;

	BroadcastSend((char*)&packet, sizeof(packet), inSession);

	//-------------------------------------------
	// session ���� (�������� delete)
	//-------------------------------------------
	
	auto iter = g_ListSession.begin();

	for (; iter != g_ListSession.end();++iter)
	{
		if ((*iter) == inSession)
		{
			delete* iter;
			//----------------------------------------
			// ���⼭ ���࿡ erase �ϸ� �ٸ� ����Ʈ���� ��ȸ�ϴ� ���������� ���ܹ�����.
			//----------------------------------------
			*iter = nullptr;
			break;
		}

	}
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
			int sendRtn = send(session->socket, buffer, len, 0);
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
	// �ּ� ���ε�
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

void ClearSession()
{

	auto iter = g_ListSession.begin();

	for (; iter != g_ListSession.end(); )
	{
		if (*iter == nullptr)
		{
			iter = g_ListSession.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	long dwStyle;

	switch (iMessage)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		hdc = GetDC(hWnd);
		TextOut(hdc, 100, 50, g_TextoutBuffer1, wcslen(g_TextoutBuffer1));
		ReleaseDC(hWnd, hdc);
		EndPaint(hWnd, &ps);
		break;
	}
	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

