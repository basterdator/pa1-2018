#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_ADDRESS_STR "127.0.0.1"
#define IN_PORT 2345
#define MAX_CLIENTS 1
#define NETWORK_ERROR -1
#define NETWORK_OK     0
#define MSG_SIZE 64
#define ORIG_SIZE 49


typedef enum { TRNS_FAILED, TRNS_DISCONNECTED, TRNS_SUCCEEDED } TransferResult_t;

typedef struct {  // block of 8X8 bits
	unsigned char wrd[8];
}blk;

typedef struct { // data structure of 1 bit, can hold 0 or 1
	unsigned short value : 1;
}bit;

SOCKET udp_s;
unsigned long Address;
struct sockaddr_in my_addr;
struct sockaddr_in sender_addr;
int sender_addr_len = sizeof(sender_addr);
struct sockaddr_in reciever_addr;
//unsigned char recieved_buffer[ORIG_SIZE];
unsigned char recieved_buffer[MSG_SIZE];




void ReportError(int errorCode, const char *whichFunc);

int main(int argc, char* argv[])
{

	int nret;
	// initialize windows networking
	WSADATA wsaData;
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

	const char* CurPlacePtr;
	int BytesJustTransferred;
	int RemainingBytesToReceive;
	while (1)	// each iteration of this endless loop recieves 8 blocks 0f 8*8 bits, handles the pairity checks and fixes,
				// decodes them to 8 blocks of 7*7 bits and saves them back to a file.
	{
		printf("Waiting for data...\n\n");
		CurPlacePtr = recieved_buffer;
		//blk* CurPlacePtr = recieved_buffer;
		//RemainingBytesToReceive = MSG_SIZE; // the data the socket recieves is 64 bytes long
		RemainingBytesToReceive = ORIG_SIZE; // the data the socket recieves is 64 bytes long
											 //	int RemainingBytesToReceive = MSG_SIZE; // the data the socket recieves is 64 bytes long
		while (RemainingBytesToReceive > 0)
		{
			// BytesJustTransferred = recvfrom(udp_s, recieved_buffer, MSG_SIZE, 0, (SOCKADDR*)&sender_addr, &sender_addr_len);
			BytesJustTransferred = recvfrom(udp_s, recieved_buffer, ORIG_SIZE, 0, (SOCKADDR*)&sender_addr, &sender_addr_len);
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

}

void ReportError(int errorCode, const char *whichFunc)
{
	char errorMsg[92];					// Declare a buffer to hold the generated error message
	ZeroMemory(errorMsg, 92);			// Automatically NULL-terminate the string The following line copies the phrase, whichFunc string, and integer errorCode into the buffer
	sprintf(errorMsg, "Call to %s returned error %d!", (char *)whichFunc, errorCode);
	MessageBox(NULL, errorMsg, "socketIndication", MB_OK);
}
