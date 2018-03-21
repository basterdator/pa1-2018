#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif


#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")


#define REMOTE_HOST_IP "127.0.0.1"
#define IN_PORT 2345
#define MAX_CLIENTS 1
#define NETWORK_ERROR -1
#define NETWORK_OK     0
#define MSG_SIZE 64
typedef enum { TRNS_FAILED, TRNS_DISCONNECTED, TRNS_SUCCEEDED } TransferResult_t;
typedef struct {  // block of 8X8 bits
	unsigned char wrd[8];
}blk;

typedef struct { // data structure of 1 bit, can hold 0 or 1
	unsigned short value : 1;
}bit;

void ReportError(int errorCode, const char *whichFunc);

int main(int argc, char* argv[])
{
	// initialize windows networking
	WSADATA wsaData;
	SOCKET s;
	struct sockaddr_in remote_addr;

	int nret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nret != NO_ERROR)
		printf("Error at WSAStartup()\n");

	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET)
	{
		nret = WSAGetLastError();		// Get a more detailed error
		ReportError(nret, "socket()");		// Report the error with our custom function
		WSACleanup();				// Shutdown Winsock
		return NETWORK_ERROR;			// Return an error value
	}

	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = inet_addr(REMOTE_HOST_IP);
	remote_addr.sin_port = htons(IN_PORT);

	blk *pta = (blk *)calloc(8 , sizeof(blk));
	const char send_buf[] = "theseare64bytesyesyesthoseare64bytesforsuretheseare64bytesyesye";
	int sent = sendto(s, send_buf, MSG_SIZE, 0, (SOCKADDR*)&remote_addr, sizeof(remote_addr) );
	if (sent == SOCKET_ERROR)
	{
		nret = WSAGetLastError();
		ReportError(nret, "sendto()");
		WSACleanup();
	}

	printf(" --> %d Sent\n", sent);
	closesocket(s);
	WSACleanup();
	return 0;
}


void ReportError(int errorCode, const char *whichFunc)

{
	char errorMsg[92];					// Declare a buffer to hold
										// the generated error message
	ZeroMemory(errorMsg, 92);				// Automatically NULL-terminate the string
											// The following line copies the phrase, whichFunc string, and integer errorCode into the buffer
	sprintf(errorMsg, "Call to %s returned error %d!", (char *)whichFunc, errorCode);
	MessageBox(NULL, errorMsg, "socketIndication", MB_OK);

}
