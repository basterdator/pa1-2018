#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_ADDRESS_STR "127.0.0.1"
#define IN_PORT 2345
#define MAX_CLIENTS 1
#define NETWORK_ERROR -1
#define NETWORK_OK  0
#define MSG_SIZE 8

char input_str[256];

HANDLE hThread[2];
SOCKET udp_s;
struct sockaddr_in my_addr;
struct sockaddr_in sender_addr;
int sender_addr_len = sizeof(sender_addr);
char recieved_buffer[MSG_SIZE];
unsigned long Address;
int nret;

typedef struct
{
	SOCKET sock;
	char *path;
}RecvThreadParms;


void ReportError(int errorCode, const char *whichFunc);
static DWORD InputThread(void);
static DWORD DataThread(LPVOID lpParam);

int main(int argc, char* argv[])
{
	// initialize windows networking
	WSADATA wsaData;
	FILE *f_dst = fopen(argv[2], "wb");  // create the file to be written into
	fclose(f_dst);

	nret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nret != NO_ERROR)
		printf("Error at WSAStartup()\n");


	// create the socket that will listen for incoming TCP connections
	udp_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_s == INVALID_SOCKET)
	{
		nret = WSAGetLastError();		
		ReportError(nret, "socket()");		
		WSACleanup();				
		return NETWORK_ERROR;			
	}

	Address = inet_addr(SERVER_ADDRESS_STR);
	if (Address == INADDR_NONE)
	{
		nret = WSAGetLastError();
		ReportError(nret, "address convertion");
		WSACleanup();
		return NETWORK_ERROR;
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = Address;
	my_addr.sin_port = htons(atoi(argv[1]));
	nret = bind(udp_s, (SOCKADDR*)&my_addr, sizeof(my_addr));
	if (nret == SOCKET_ERROR)
	{
		nret = WSAGetLastError();
		ReportError(nret, "bind()");
		WSACleanup();
		return NETWORK_ERROR;
	}

	hThread[0] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)InputThread,
		NULL,
		0,
		NULL
	);

	hThread[1] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)DataThread,
		argv[2],
		0,
		NULL
	);

	WaitForMultipleObjects(2, hThread, FALSE, INFINITE);

	TerminateThread(hThread[0], 0x555);
	TerminateThread(hThread[1], 0x555);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	closesocket(udp_s);
	WSACleanup();

	return 0;
}

void ReportError(int errorCode, const char *whichFunc)

{
	char errorMsg[92];					// Declare a buffer to hold the generated error message
	ZeroMemory(errorMsg, 92);			// Automatically NULL-terminate the string The following line copies the phrase, whichFunc string, and integer errorCode into the buffer
	sprintf(errorMsg, "Call to %s returned error %d!", (char *)whichFunc, errorCode);
	MessageBox(NULL, errorMsg, "socketIndication", MB_OK);
}

static DWORD InputThread(void)
{
	int i = 1;
	while (i)
	{
		gets_s(input_str, sizeof(input_str));
		if (strcmp(input_str, "End") == 0)
		{
			i = 0;
		}
		else
		{
			printf("Message: Unknown command\n");
		}
	}
	WSACleanup();
	return 0;

}

static DWORD DataThread(LPVOID lpParam)
{
	char *path;
	path = (char *)lpParam;

	while (1)
	{
		printf("Waiting for data...\n");
		char* CurPlacePtr = recieved_buffer;
		int BytesJustTransferred;
		int RemainingBytesToReceive = MSG_SIZE;
		while (RemainingBytesToReceive > 0)
		{
			BytesJustTransferred = recvfrom(udp_s, recieved_buffer, MSG_SIZE, 0, (SOCKADDR*)&sender_addr, &sender_addr_len);
			if (BytesJustTransferred == SOCKET_ERROR)
			{
				nret = WSAGetLastError();
				ReportError(nret, "recvfrom()");
				WSACleanup();
				return NETWORK_ERROR;
			}
			else if (BytesJustTransferred == 0)
			{
				WSACleanup();
				return NETWORK_ERROR;
			}
			RemainingBytesToReceive -= BytesJustTransferred;
			CurPlacePtr += BytesJustTransferred;
		}
		printf("Recieved buffer:\n%s\n", recieved_buffer);
		FILE *f_dst = fopen(path, "a");
		printf("%d\n", strlen(recieved_buffer));
		if (fwrite(recieved_buffer, 1, strlen(recieved_buffer), f_dst) != strlen(recieved_buffer))
		{
			printf("ERROR - Failed to write %i bytes to file\n", strlen(recieved_buffer));
			exit(1);
		}
		fclose(f_dst);
		f_dst = NULL;
	}

}
