#include "SocketLog.h"
#include <stdio.h>
#include <WinSock2.h>

void PrintErrorLog(const wchar_t* str, int line)
{
	wprintf(L"%s  [ErrorCode:%d] [Line:%d]\n",str, WSAGetLastError(),line);
}
