#include "link.h"
#include <sys/type.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

enum frameFields {
	START_FLAG, ADDR, CTRL, BCC1, DATA, BCC2, END_FLAG
};

#define TRANSMITTER 0
#define RECEIVER 1

#define FLAG 0x7E
#define ADDR_TRANS 0x03
#define ADDR__REC_RESP 0x03
#define ADDR_REC 0x01
#define ADDR_TRANS_RESP 0x01
#define CTRL_SET 0x03
#define CTRL_UA 0x07
#define CTRL_DIST 0x0B

#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

linkLayer_t link;
unsigned int isLinkInit = 0;
unsigned int isLinkOpen = 0;
unsigned int isDefaultBR = 0;
unsigned int isDefaultTO = 0;
unsigned int isDefaultNT = 0;
unsigned int isDefaultHER = 0;
unsigned int isDefaultFER = 0;

struct termios oldtio, newtio;

int alarmFlag = 1;
unsigned int alarmCount = 0;

int open_port_file(unsigned int port);
int close_port_file(int fd);
int send_set(int fd);
int rec_set(int fd);
int send_ua(int fd);
int send_disc(int fd, unsinged int flag);
int rec_disc(int fd, unsigned int flag);
void alarmListener();
int rec_ua(int fd, unsigned int flag);

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
		llclose(fd);
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

int close_port_file(int fd) {
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		return -1;
	}
	close(fd);

	return 0;
}

int send_set(int fd) {
	unsigned char SET[5];

	SET[0] = FLAG;
	SET[1] = ADDR_TRANS;
	SET[2] = CTRL_SET;
	SET[3] = (SET[1] ^ SET[2]);
	SET[4] = FLAG;

	write(fd,SET,5);

	return 0;
}

int rec_set(int fd) {
	int i = START_FLAG;

	while (STOP==FALSE) {
		unsigned char c = 0;

		if (read(fd, &c,1)) {
			switch (i) {
				case START_FLAG:
					if (c == FLAG)
						i = ADDR;
					break;
				case ADDR:
					if (c == ADDR_TRANS) {
						i = CTRL:
					} else if (c != FLAG) {
						i = START_FLAG;
					}
					break;
				case CTRL:
					if (c == CTRL_SET) {
						i = BCC1;
					} else if (c == FLAG) {
						i = ADDR;
					} else {
						i = START_FLAG;
					}
					break;
				case BCC1:
					if (c == (ADDR_TRANS ^ CTRL_SET)) {
						i = END_FLAG;
					} else if (c == FLAG) {
						i = ADDR;
					} else {
						i = START_FLAG;
					}
					break;
				case END_FLAG:
					if (c == FLAG) {
						STOP == TRUE;
					} else {
						i = START_FLAG;
					}
					break;
			}
		}
	}

	return 0;
}

int send_ua(int fd, unsigned int flag) {
	unsigned char UA[5];

	UA[0] = FLAG;

	if (flag == TRANSMITTER) {
		UA[1] = ADDR_TRANS_RESP;
	} else if (flag == RECEIVER) {
		UA[1] = ADDR_REC_RESP;
	}

	UA[2] = CTRL_UA;
	UA[3] = UA[1] ^ UA[2];
	UA[4] = FLAG;

	write(fd, UA, 5);

	return 0;
}

int send_disc(int fd, unsigned int flag) {
	unsigned char DISC[5];

	DISC[0] = FLAG;

	if (flag == TRANSMITTER)
		DISC[1] = ADDR_TRANS;
	else if (flag == RECEIVER)
		DISC[1] = ADDR_REC_RESP;

	DISC[2] = CTRL_DISC;
	DISC[3] = DISC[1] ^ DISC[2];
	DISC[4] = FLAG;

	write(fd, DISC, 5);

	return 0;
}

int rec_disc(int fd, unsigned int flag) {
	if (flag == TRANSMITTER) {
		(void) siganl(SIGALRM, alarmListener);
		alarm(link.timeout);
		alarmFlag = 1;
		alarmCount = 0;
	}

	unsigned char addr = 0;
	unsigned char ctrl = 0;

	int i = STAR_FLAG;
	STOP = FALSE;

	while (STOP == FALSE) {
		unsigned char c = 0;

		if (flag == TRANSMITTER) {
			if (alarmCount >= link.numTransmissions) {
				printf("EXCEDED NUMBER OF TRIES\n");
				close_port_file(fd);
				return -1;
			} else if (alarmFlag == 0) {
				send_disc(fd, flag);
				alarmFlag = 1;
				alarm(link.timeout);
			}
		}

		if (read(fd, &c, 1)) {
			switch (i) {
				case START_FLAG:
					if (c == FLAG)
						i = ADDR;
					break;
				case ADDR:
					if ((flag == TRANSMITTER && c == ADDR_REC_RESP) || (flag == RECEIVER && c == ADDR_TRANS)) {
						addr = c;
						i = CTRL;
					} else if (c != FLAG) {
						i = START_FLAG;
					}
					break;
				case CTRL:
					if (c == CTRL_DISC) {
						ctrl = c;
						i = BCC1;
					} else if (c == FLAG) {
						i = ADDR;
					} else {
						i = START_FLAG;
					}
					break;
				case BCC1:
					if (c == (addr ^ ctrl)) {
						i = END_FLAG;
					} else if (c == FLAG) {
						i = ADDR;
					} else {
						i = START_FLAG;
					}
					break;
				case END_FLAG:
					if (c == FLAG) {
						STOP = TRUE;
					} else {
						i = START_FLAG;
					}
					break;
			}
		}
	}

	return 0;
}

void alarmListener() {
	alarmFlag = 1;
	alarmCount++;
}

int rec_ua(int fd, unsigned int flag) {
	(void) signal(SIGALRM, alarmListener);
	alarm(link.timeout);

	alarmFlag = 1;
	alarmCount = 0;

	unsigned int addr = 0;
	unsigned int ctrl = 0;

	int i = START_FLAG;
	STOP = FALSE;

	while (STOP == FALSE) {
		unsigned char c = 0;

		if (alarmCount >= link.numTransmissions) {
			printf("EXCEDED NUMBER OF TRIES!\n");
			close_port_file(fd);
			return -1;
		} else if (alarmFlag == 0) {
			if (flag == TRANSMITTER) {
				send_set(fd);
			} else if (flag == RECEIVER) {
				send_disc(fd, flag);
			}

			alarmFlag = 1;
			alarm(link.timeout);
		}

		if (read(fd, &c, 1)) {
			switch (i) {
				case START_FLAG:
					if (c == FLAG)
						i = ADDR;
					break;
				case ADDR:
					if ((flag == TRANSMITTER && c == ADDR_REC_RESP) || (flag == RECEIVER && c == ADDR_TRANS_RESP)) {
						addr = c;
						i = CTRL;
					} else if (c != FLAG) {
						i = START_FLAG;
					}
					break;
				case CTRL:
					if (c == CTRL_UA) {
						ctrl = c;
						i = BCC1;
					} else if (c == FLAG) {
						i = ADDR;
					} else {
						i = START_FLAG;
					}
					break;
				case BCC1:
					if (c == (addr ^ ctrl)) {
						i = END_FLAG;
					} else if (c == FLAG) {
						i = ADDR;
					} else {
						i = START_FLAG;
					}
					break;
				case END_FLAG:
					if (c == FLAG) {
						STOP = TRUE;
					} else {
						i = START_FLAG;
					}
					break;
			}
		}
	}

	return 0;
}
