<<<<<<< HEAD
#include "link.h"
#include <sys/type.h>
=======
#include <sys/types.h>
>>>>>>> origin/master
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
<<<<<<< HEAD
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>


linkLayer_t link;
unsigned int isLinkInit = 0;
unsigned int isLinkOpen = 0;
unsigned int isDefaultBR = 0;
unsigned int isDefaultTO = 0;
unsigned int isDefaultNT = 0;
unsigned int isDefaultHER = 0;
unsigned int isDefaultFER = 0;

struct termios oldtio, newtio;

int open_port_file(unsigned int port);
int send_set(int fd);

int llopen(unsigned int port, unsigned int flag) {
	if (init_link(port))
		return -1;

	link.status = flag;
	int fd = open_port_file(port);

	if (flag == RECEIVER) {
		if (receive_set(fd))
			return -1;
		if (send_ua(fd, flag))
			return -1;
	} else if (flag == TRANSMITTER) {
		if (send_set(fd))
			return -1;
		if (receive_ua(fd, flag))
			return -1;
	} else {
		printf("ENTER A VALID STATUS!\n");
		close_port_file(fd);
		return -1;
	}

	isLinkInit = -1;
	return fd;
}

int init_link(unsigned int port) {
	if (port<0 || port>4) {
		printf("ENTER A VALID PORT!\n");
		return -1;
	}

	sprintf(link.port, "/dev/ttyS%d", port);

	if (!isDefaultBR)
		link.baudRate = B38400;
	link.sequenceNumber = 0;
	if (!isDefaultTO)
		link.timeout = 3;
	if (!isDefaultNT)
		link.numTransmissions = 3;
	link.isLinkOpen = -1;
	if (!isDefaultHER)
		link.her = 0;
	if (!isDefaultFER)
		link.fer = 0;

	return 0;
}

int open_port_file(unsigned int port) {
	int fd;

	fd = open(link.port, O_RDWD | O_NOCTTY);
	if (fd < 0) {
		perror(link.port);
		close_port_file(fd);
		return -1;
	}

	if (tcgetattr(fd,&oldtio) == -1) {
		perror("tcgetattr");
		close_port_file(fd);
		return -1;
	}

	bzero(&nwetio, sizeof(newtio));
	newtio.c_cflag = link.baudRate | CS8 | CSLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = OPOST;

	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 1;

	tcflush(fd, TCIFLUSH);

	if(tcsetattr(fd, TCSANOW, &newtio) == -1) {
		perror("tcsetattr");
		close_port_file(fd);
		return -1;
	}

	return fd;
}

int send_set(int fd) {
	unsigned char SET[5];

	SET[0] = FLAG;
	SET[1] = ADDR;
	SET[2] = C_SET;
	SET[3] = (SET[1] ^ SET[2]);
	SET[4] = FLAG;

	write(fd,SET,5);

	return 0;
}
=======

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define F 0x7e
#define A 0x03
#define UA 0x07
volatile int STOP=FALSE;


int ll_open()
>>>>>>> origin/master
