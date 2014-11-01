#ifndef _LINK_H_
#define _LINK_H_

typedef struct linkLayer {
	char port[20];
	unsigned int baudRate;
	unsigned int sequenceNumber;
	unsigned int timeout;
	unsigned int numTransmissions;
} linkLayer_t;

ll_open(unsigned int port, unsigned int flag);

