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
#define MAX_CLIENTS 1
#define NETWORK_ERROR -1
#define NETWORK_OK  0
#define MSG_SIZE 64

#define MSB1 128
#define MSB0 127
#define LSB1 1
#define LSB0 254

const int powers2[8] = { 1 ,2, 4, 8, 16, 32, 64, 128 };


typedef struct {  // block of 8X8 bits
	unsigned char wrd[8];
}blk;

typedef struct { // data structure of 1 bit, can hold 0 or 1
	unsigned short value : 1;
}bit;

char input_str[256];

HANDLE hThread[2];
SOCKET udp_s;
struct sockaddr_in my_addr;
struct sockaddr_in sender_addr;
int sender_addr_len = sizeof(sender_addr);
const char recieved_buffer[49];
//blk recieved_buffer[MSG_SIZE/8];
unsigned long Address;
int nret;
blk fixed_blks[8];
int num_of_bytes_recieved;
int num_of_errors = 0;
int num_fixed = 0;



void ReportError(int errorCode, const char *whichFunc);
static DWORD InputThread(void);
static DWORD DataThread(LPVOID lpParam);
int line_parity_check(unsigned char line);
void count_and_fix(blk in_blks[8], blk *out_blks, int *nerr, int *nfix);


int main(int argc, char* argv[])
{
	FILE *f_dst = fopen(argv[2], "wb");  // create the file to be written into
	fclose(f_dst);

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
		(LPTHREAD_START_ROUTINE)InputThread, // A thread which recieves data on the socket and proccesses it
		NULL,
		0,
		NULL
	);

	hThread[1] = CreateThread( 
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)DataThread, // A thread which listens to the users input and acts accordingly when "End" is given
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
	while (1)
	{
		gets_s(input_str, sizeof(input_str));
		if (strcmp(input_str, "End") == 0)
		{
			// do all the stuff that sends the info to the channel and to the user
			WSACleanup();
			return 0;
		}
		else
		{
			printf("Message: Unknown command\n");
		}
	}
}

static DWORD DataThread(LPVOID lpParam)
{
	char *path;
	path = (char *)lpParam;
	while (1)	// each iteration of this endless loop recieves 8 blocks 0f 8*8 bits, handles the pairity checks and fixes,
				// decodes them to 8 blocks of 7*7 bits and saves them back to a file.
	{
		printf("Waiting for data...\n");
		const char* CurPlacePtr = recieved_buffer;
		//blk* CurPlacePtr = recieved_buffer;
		int BytesJustTransferred;
		int RemainingBytesToReceive = 49; // the data the socket recieves is 64 bytes long
	//	int RemainingBytesToReceive = MSG_SIZE; // the data the socket recieves is 64 bytes long
		while (RemainingBytesToReceive > 0)
		{
			// BytesJustTransferred = recvfrom(udp_s, recieved_buffer, MSG_SIZE, 0, (SOCKADDR*)&sender_addr, &sender_addr_len);
			BytesJustTransferred = recvfrom(udp_s, recieved_buffer, 49, 0, (SOCKADDR*)&sender_addr, &sender_addr_len);
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
		printf("DATA: \n");               // print the data
		for (int c = 0; c < 49; c++) {
			if (c % 7 == 0) {
				printf("\n");
			}
			printf("%0X ", recieved_buffer[c]);
		}
		printf("\n\n");


		//printf("\n\nENCODED:\n"); // print the encoded data
		//for (int m = 0; m < 1; m++) {
		//	for (int p = 0; p < 8; p++) {
		//		for (int w = 0; w < 8; w++) {
		//			printf("%02X ", recieved_buffer[m * 8 + p].wrd[w]);
		//		}
		//		printf("\n");
		//	}
		//	printf("\n\n");
		//}

		//count_and_fix(recieved_buffer, &fixed_blks, &num_of_errors, &num_fixed); //
		
		//printf("\n\nFIXED:\n"); // print the fixed data
		//for (int m = 0; m < 1; m++) {
		//	for (int p = 0; p < 8; p++) {
		//		for (int w = 0; w < 8; w++) {
		//			printf("%02X ", fixed_blks[m * 8 + p].wrd[w]);
		//		}
		//		printf("\n");
		//	}
		//	printf("\n\n");
		//}

		//printf("ERRORS: %d\nFIXED: %d\n", num_of_errors, num_fixed);

		/// break down the buffer to 8 blks
		/// for each blk: calc_pairity_and_fix()

		/// decode()

		/// send to channel



		FILE *f_dst = fopen(path, "ab");
		//printf("%d\n", strlen(recieved_buffer));
		if (fwrite(recieved_buffer, 1, sizeof(recieved_buffer), f_dst) != sizeof(recieved_buffer))
		{
			printf("ERROR - Failed to write %i bytes to file\n", MSG_SIZE);
			exit(1);
		}
		fclose(f_dst);
		f_dst = NULL;
	}

}

/**************************************
function:		count_and_fix
input parm:		a pointer to an array of blks (size: 8 blks (64B))
output parms:	a pointer to an integer that counts the errors
				a pointer to an integer which count the number of fixable bytes
functionality:	iterate over each blk, count the number of errors in each blk and fix it if it's possible
***************************************/
void count_and_fix(blk in_blks[8], blk *out_blks, int *nerr, int *nfix) {
	int i, j, k;
	int sum_lines = 0;
	int sum_cols = 0;
	int detected = 0;
	int fixed = 0;
	int lines[] = { 0,0,0,0,0,0,0,0 };
	int columns[] = { 0,0,0,0,0,0,0,0 }; // smallest index represents the LSB of the column parity byte
	//blk temp_blks[8];
	unsigned char c_pair_chk;
	unsigned char fix_byte;

	for (i = 0;i < 8;i++) { // for each block in the incoming array
		*(out_blks + i) = in_blks[i];  // save the current blk as-is, we will fix it if we can
		for (j = 0;j < 8;j++) { // for each row in the block
			lines[j] = line_parity_check(in_blks[i].wrd[j]); // for 0<j<7, lines[j]=1 indicates that the pairty in that row is broken
			sum_lines += lines[j]; // count how many bits are equal to 1 in the pairity check result row vector
		}
		/// xor all the rows in order to get the xor of each column
		c_pair_chk = 0; //  c_pair_chk is set to be 0000 0000
		for (j = 0; j < 8; j++) {
			c_pair_chk ^= in_blks[i].wrd[j]; // bitwise xor of all lines
		}
		for (j = 0; j < 8; j++) {  // iterate over the bits of c_pair_chk and put each bit in the corresponding location in the columnes vector
			columns[j] = c_pair_chk & 1; // leave only the LSB and store it in columns[]
			c_pair_chk = c_pair_chk / 2;  // shift the byte right
			sum_cols += columns[j]; // count how many bits are equal to 1 in the pairity check result column vector
		}
		if (sum_cols == 1 && sum_lines == 1) {
			detected++;
			fixed++;
			// fix()
			k = 0;
			while (columns[k] == 0)  // this loop finds which bit we want to fix within the line
			{
				k++;
			}
			fix_byte = powers2[k]; // we create a byte with "1" only in the location of the bit we want to invert
			k = 0;
			while (lines[k] == 0)  // this loop finds the number of line we want to fix
			{
				k++;
			}
			*(out_blks+i)->wrd ^= fix_byte; // xor the broken bit with 1 in order to invert it, xor the rest of the bits with 0 to keep them the same
		}
		else if (sum_cols != 0 && sum_lines != 0) {
			detected++; // if the number of pairity errors is not (1,1) or (0,0) we consider it to be an unfixable error which we count as detected.
		}
	}
	*nerr += detected;
	*nfix += fixed;
}


/**************************************
function: parity
input: unsigned char line (8bits)
output: unsigned char line (8bits)
functionality: change the LSB according
to parity of the other 7 bits.
(the LSB is destroyed, and use in other
line.
***************************************/
int line_parity_check(unsigned char line) {
	bit paritybit = { 0 }; // define the parity bit
	for (int b = 0; b < 8; b++) {
		paritybit.value ^= ((line & powers2[b]) / powers2[b]);
	}
	return paritybit.value;
}

