#ifndef APPLICATION_H_
#define APPLICATION_H_



#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "link.h"
#include "definitions.h"

struct app_layer {
	int status;
	int port;
	int serialPortDescriptor;
	int fileDescriptor;
	int packetSize;
	int fileSize;
	unsigned char currentNum;
	unsigned char name[PATH_MAX];
}typedef app_layer_T;

int cliPort();
int cliStatus();
int startApp();
int setup();
int sendFile();
int start_settings();
int receiveFile();
int verifyFile(unsigned char * packet, unsigned int size);
int processPacket(unsigned char * packet);
int writeToFile(unsigned char * info, unsigned int byn_number);
int createFile(char * name);
int receiveControl(int var);
int sendPacket(unsigned char * message, unsigned int size);
int sendControl(int var);
int cliPacket();
int cliBaudrate();
int cliRetries();
char* cliChooseFile();
int getFileSize(char* path);
int validPath(unsigned char * path);
int cliTimeout();



#endif
