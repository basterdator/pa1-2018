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

}


void ReportError(int errorCode, const char *whichFunc)

{
	char errorMsg[92];					// Declare a buffer to hold the generated error message
	ZeroMemory(errorMsg, 92);			// Automatically NULL-terminate the string The following line copies the phrase, whichFunc string, and integer errorCode into the buffer
	sprintf(errorMsg, "Call to %s returned error %d!", (char *)whichFunc, errorCode);
	MessageBox(NULL, errorMsg, "socketIndication", MB_OK);
}