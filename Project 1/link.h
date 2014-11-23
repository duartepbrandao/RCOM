#ifndef _LINK_H_
#define _LINK_H_

typedef struct linkLayer {
	char port[20];
	unsigned int baudRate;
	unsigned int sequenceNumber;
	unsigned int timeout;
	unsigned int numTransmissions;
	unsigned int openLink;
	int flag;
	unsigned int her;
	unsigned int fer;
} linkLayer_t;

void setAlarm();
int llopen(unsigned int port, unsigned int flag);
int llwrite(int fd, unsigned char * buffer, unsigned int length);
int llread(int fd, unsigned char * buffer);
int llclose(int fd, unsigned int flag);
#endif
