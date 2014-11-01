#ifndef APPLICATION_H_
#define APPLICATION_H_



#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "definitions.h"

struct app_layer {
	int status;
	int port;
	int serialPortDescriptor;
	int packetSize;
	int fileDescriptor;
	int fileSize;
	unsigned char currentNum;
	unsigned char name[PATH_MAX];
} typdef app_layer_T;

int cliPort();
int cliStatus();
int startApp();
int setup();
int sendFile();
int start_settings;
int receiveFile();


#endif
