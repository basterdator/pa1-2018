#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define MSB1 128
#define MSB0 127
#define LSB1 1
#define LSB0 254

#define MAX_CLIENTS 1
#define NETWORK_ERROR -1
#define NETWORK_OK     0
typedef enum { TRNS_FAILED, TRNS_DISCONNECTED, TRNS_SUCCEEDED } TransferResult_t;

void ReportError(int errorCode, const char *whichFunc);


const int powers2[8] = { 1 ,2, 4, 8, 16, 32, 64, 128 };

typedef struct {  // block of 8X8 bits
	unsigned char wrd[8];
}blk;

typedef struct { // data structure of 1 bit, can hold 0 or 1
	unsigned short value : 1;
}bit;

// prototype of functions
unsigned char parity(unsigned char p);
blk blockparity(blk b);
blk* encode(unsigned char data[49]);


int main(int argc, char* argv[])
{
	size_t read_bytes; // stores the number of bytes that were read from the stream (supposed to be 49, an error is thrown in case something goes wrong)
	int nret; // the return value dumpster, used for error checking
	unsigned long Address; // used to store the destination address of the data
	FILE * fp;
	WSADATA wsaData;
	SOCKET s;
	struct sockaddr_in remote_addr;
	unsigned char buffer[49]; // a buffer for reading from the file
	unsigned char *p_buffer; // a pointer to the buffer, used in fread
	unsigned char ret_buf[4];
	int BytesJustTransferred;
	struct sockaddr_in sender_addr;
	int sender_addr_len = sizeof(sender_addr);


							 /// *** Socket Initiation ***  
	nret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nret != NO_ERROR)
		printf("Error at WSAStartup()\n");

	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET)
	{
		nret = WSAGetLastError();
		ReportError(nret, "socket()");
		WSACleanup();
		return NETWORK_ERROR;
	}
	Address = inet_addr(argv[1]);
	printf("%lu\n", Address);
	if (Address == INADDR_NONE)
	{
		nret = WSAGetLastError();
		ReportError(nret, "address convertion");
		WSACleanup();
		return NETWORK_ERROR;
	}
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = Address;
	remote_addr.sin_port = htons(atoi(argv[2]));


	// open the file to a stream, divide it to chunks of 49 blocks
	fp = fopen(argv[3], "rb");
	p_buffer = &buffer;

	read_bytes = fread(p_buffer, 1, 49, fp);

	while (read_bytes == 49)
	{
		// encode(read_bytes, send_buffer )
		//blk *send_buffer = (blk *)calloc(8, sizeof(blk)); // create an 8 blk array that will store the converstion result
		//send_buffer = encode(buffer);
		///// print for later comparison
		//printf("%d", sizeof(send_buffer));
		printf("DATA: \n");               // print the data
		for (int c = 0; c < 49; c++) {
			if (c % 7 == 0) {
				printf("\n");
			}
			printf("%0X ", buffer[c]);
		}
		printf("\n");

		// int sent = sendto(s, send_buffer, 8*sizeof(send_buffer), 0, (SOCKADDR*)&remote_addr, sizeof(remote_addr));
		int sent = sendto(s, buffer, 49, 0, (SOCKADDR*)&remote_addr, sizeof(remote_addr));
		if (sent == SOCKET_ERROR)
		{
			nret = WSAGetLastError();
			ReportError(nret, "sendto()");
			WSACleanup();
		}
		read_bytes = fread(p_buffer, 1, 49, fp); // read the next 49 bits
	}

	//nret = bind(s, (SOCKADDR*)&remote_addr, sizeof(remote_addr));
	//if (nret == SOCKET_ERROR)
	//{
	//	nret = WSAGetLastError();
	//	ReportError(nret, "bind()");
	//	WSACleanup();
	//	return NETWORK_ERROR;
	//}

	BytesJustTransferred = recvfrom(s, ret_buf, 4, 0, (SOCKADDR*)&sender_addr, &sender_addr_len);
	if (BytesJustTransferred == SOCKET_ERROR)
	{
		nret = WSAGetLastError();
		ReportError(nret, "recvfrom()");
		WSACleanup();
		return NETWORK_ERROR;
	}
	printf("\nreceived: %u bytes\nwritten: %u bytes\ndetected: %u errors, corrected: %u errors\n", ret_buf[0], ret_buf[1], ret_buf[2], ret_buf[3]);

	/// send each block to the channel until we reach the end of the file
	/// once the entire data is sent, listen to the feedback from the reciever

	return 0;
}
/**************************************
function:		encode
input:			unsigned char data (49 bytes long)
output:			blk pta (the encoded 8 blks)
functionality:	convert blocks of 7x7 arrays of data to blks of 8x8 arrays of data (adding pairity bits to each row and column)
***************************************/
blk* encode(unsigned char data[49]) {
	//unsigned char data[49] = {	250,57,91,28,39,164,86, // example data for testing
	//							178,80,61,46,175,228,112,
	//							91,22,178,196,235,193,20,
	//							163,139,39,212,28,37,33,
	//							242,188,21,54,212,74,40,
	//							31,46,66,181,193,27,53,
	//							65,63,91,44,179,104,66 };
	//printf("DATA: \n");               // print the data
	//for (int c = 0; c < 49; c++) {
	//	if (c % 7 == 0) {
	//		printf("\n");
	//	}
	//	printf("%0X ", data[c]);
	//}
	int M = 49 / 49;         // M is len of data in bytes divided by 49 ////// !!!!!!!!!!!!!!!!!!!!!!!!
	blk *pta = (blk *)calloc(8 * M, sizeof(blk));
	bit keepbit[7];    // array of bits that need to be keep for next line
	bit keptbit[7];    // array of bits that kept from the previous line for using in current line
	int m = 0, p = 0, j = 0;    // inedxes for loops initial to 0
	unsigned char keepword[7];  // word - 8 bits - a line in the block. array of 8 words for the next block
	unsigned char keptword[7];  // array of 8 words that kept from the previous block for using in the current block
	for (m = 0; m < M; m++) { // number of periodic blocks // M = len(bytes)/49
		for (p = 0; p < 8; p++) { // the data is integer multy- of 8 groups of 49 bits
			blk *dib = NULL;
			dib = pta + p + (7 * m);
			for (int pp = 0; pp < p; pp++) { //  in each group there are several word reserved from the previous group
				dib->wrd[pp] = keptword[pp];  // restore words that unused in the previous block
			}
			if (p == 7) // in the 8th block, all the line are saved from previous block
				continue;
			for (j = 0; j < 8; j++) { // loop for dealing with 1 byte from data
				unsigned char w = '\0';  // word, will be 7 bit+parity
				if (j == 7) { // in each 7-bytes the last 7 bits will be word that need to keep for the next block - whole the word is keptbit bits 
					for (int n = 0; n < 7; n++) {
						w += powers2[(7 - n)] * keptbit[n].value;
					}
				}
				else {
					w = data[7 * m + 7 * p + j]; // w is one byte from the data
					for (int k = j; k >= 0; k--) {  // keep on bits for use in the next word
						keepbit[k].value = (w / powers2[(j - k)]) & 1;
					}
					w = w / powers2[j];
					for (int k = (j - 1); k >= 0; k--) { // restore bits that unused in the previous word
						w += powers2[(7 - k)] * keptbit[k].value;
					}
				}
				if ((p + j) <= 6) {
					dib->wrd[j + p] = w; // the last bit will be change according to parity
				}
				else {
					keepword[p + j - 7] = w; // only 7 words in use in each block, the other will be kept for next block, in the 3rd block there is 2 kept word etc.
				}
				for (int l = 0; l <= j; l++) {
					keptbit[l].value = keepbit[l].value; // before continue to next line, keep the bits that need to keep
					keepbit[l].value = 0; // and clean the array for the next line
				}
			} // for j
			for (int t = 0; t <= p; t++) {
				keptword[t] = keepword[t];  // before continue to next block, keep the words that need to keep
				keepword[t] = '\0';   // and clean the array for the next block
			}
		} // for p
		for (int p = 0; p < 8; p++) { // for each block, call to blockparity to add parity bit in the end of each line, and parity line for the whole block
			blk *pb;
			pb = pta + (7 * m) + p;
			*pb = blockparity(*pb);
		}
	} // for m

	  //printf("\n\nENCODED:\n"); // print the encoded data
	  //for (int m = 0; m < M; m++) {
	  //	for (int p = 0; p < 8; p++) {
	  //		for (int w = 0; w < 8; w++) {
	  //			printf("%02X ", pta[m * 8 + p].wrd[w]);
	  //		}
	  //		printf("\n");
	  //	}
	  //	printf("\n\n");
	  //}
	return pta;
} // encode

  /**************************************
  function: parity
  input: unsigned char line (8bits)
  output: unsigned char line (8bits)
  functionality: change the LSB according
  to parity of the other 7 bits.
  (the LSB is destroyed, and use in other
  line.
  ***************************************/
unsigned char parity(unsigned char line) {
	line = line & LSB0;
	bit paritybit = { 0 };
	for (int b = 1; b < 8; b++) {
		paritybit.value ^= ((line & powers2[b]) / powers2[b]);
	}
	return line^paritybit.value;
}

/**************************************
function: blockparity
input: block (8X8 bits)
output: block (8X8 bits)
functionality: call to parity function
for each line. and add the 8th line that
every bit is the parity of his column.
then call to parity for the 8th line.
***************************************/
blk blockparity(blk b) {
	unsigned char pw = '\0';
	char pl = '\0';
	for (int l = 0; l <= 6; l++) {
		b.wrd[l] = parity(b.wrd[l]);
		pl ^= b.wrd[l];
	}
	b.wrd[7] = parity(pl);
	return b;
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


blk AddNoise(blk b, unsigned int n, unsigned int seed) {
	srand(seed);
	int flip = 0, count = 0;
	unsigned int r0 = rand();
	unsigned int r[2];
	for (int j = 0; j < 8; j++) {
		if (j < 2)
			r[j] = 0xFFFFFFFF;
		r[j % 2] &= (rand()*(int)pow(2, 17) + rand() * 4 + ((r0 & (int)pow(2, j) * 3) >> j) ^ (r[1 - j % 2] & (int)pow(2, 14 - j) * 3 >> (14 - j))); // + random (0,3)
	}

	printf("\nBLOCK:\n%02X%02X%02X%02X\n%02X%02X%02X%02X\n", b.wrd[0], b.wrd[1], b.wrd[2], b.wrd[3], b.wrd[4], b.wrd[5], b.wrd[6], b.wrd[7]);
	printf("\nRAND:\n%08X\n%08X\n", r[0], r[1]);

	for (int bit = 0; bit < 64; bit++) {
		int rem32 = bit % 32; int rem8 = bit % 8;
		int div32 = bit / 32; int div8 = bit / 8;
		if ((((r[div32]) & ((int)pow(2, rem32))) >> rem32) == 1) {
			count++;
			if ((int)pow(2, rem8)*(rand() < 16 * n)) {
				b.wrd[3 - div8 + 8 * div32] ^= 1;
				flip++;
			}
		}
	}
	printf("\nNOISE:\n%02X%02X%02X%02X\n%02X%02X%02X%02X\n", b.wrd[0], b.wrd[1], b.wrd[2], b.wrd[3], b.wrd[4], b.wrd[5], b.wrd[6], b.wrd[7]);
	printf("\ncount: %d\nflip : %d", count, flip);

	return b;
}