#include "link.h"
#include <sys/types.h>
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

#define SENDER 0
#define RECEIVER 1

#define FLAG 0x7E
#define ADDR_TRANS 0x03
#define ADDR_REC_RESP 0x03
#define ADDR_REC 0x01
#define ADDR_TRANS_RESP 0x01
#define CTRL_SET 0x03
#define CTRL_UA 0x07
#define CTRL_DISC 0x0B

#define FALSE 0
#define TRUE 1

#define NEXT_INDEX(num) (num ^ 0x01)

#define OCTET_ESCAPE 0x7D
#define OCTET_DIFF 0x20

#define MAX_SIZE 256000

#define NEXT_CTRL_INDEX(num) (num << 1)
#define CTRL_REC_READY(num) ((num << 5) | 0x05)
#define CTRL_REC_REJECT(num) ((num << 1) | 0x01)

volatile int STOP=FALSE;

unsigned int initialized = 0;
unsigned int openLink = 0;

linkLayer_t linklayer;
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

int init_link(unsigned int port);
int open_port_file(unsigned int port);
int close_port_file(int fd);
int send_set(int fd);
int rec_set(int fd);
int send_ua(int fd, unsigned int flag);
int send_disc(int fd, unsigned int flag);
int rec_disc(int fd, unsigned int flag);
void alarmListener();
int rec_ua(int fd, unsigned int flag);
int generate_bcc2(unsigned char * buffer, unsigned int length, unsigned char * bcc2);
int send_data(int fd, unsigned char * buffer, unsigned int length);
int byteStuffing(unsigned char * buffer, unsigned int length, unsigned char * new_buffer);
int rec_resp_receiver(int fd, unsigned char * buffer, unsigned int length, unsigned char bcc2);
int rec_data(int fd, unsigned char * buffer);
int byteDestuffing(unsigned char * buffer, unsigned int length, unsigned char * new_buffer);
int send_rej(int fd);
int send_rr(int fd);
int setBR(unsigned int baudrate);
int setTO(unsigned int timeout);
int setNT(unsigned int numtransmissions);
int setHER(unsigned int her);
int setFER(unsigned int fer);

int llopen(unsigned int port, unsigned int flag) {
	if (init_link(port))
		return -1;

	linklayer.flag = flag;
	int fd = open_port_file(port);

	if (flag == RECEIVER) {
		if (rec_set(fd))
			return -1;
		if (send_ua(fd, flag))
			return -1;
	} else if (flag == SENDER) {
		if (send_set(fd))
			return -1;
		if (rec_ua(fd, flag))
			return -1;
	} else {
		printf("ENTER A VALID STATUS!\n");
		close_port_file(fd);
		return -1;
	}

	initialized = -1;
	return fd;
}

int init_link(unsigned int port) {
	if (port<0 || port>4) {
		printf("ENTER A VALID PORT!\n");
		return -1;
	}

	sprintf(linklayer.port, "/dev/ttyS%d", port);

	if (!isDefaultBR)
		linklayer.baudRate = B38400;
	linklayer.sequenceNumber = 0;
	if (!isDefaultTO)
		linklayer.timeout = 3;
	if (!isDefaultNT)
		linklayer.numTransmissions = 3;
	linklayer.openLink = -1;
	if (!isDefaultHER)
		linklayer.her = 0;
	if (!isDefaultFER)
		linklayer.fer = 0;

	return 0;
}

int open_port_file(unsigned int port) {
	int fd;

	fd = open(linklayer.port, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror(linklayer.port);
		close_port_file(fd);
		return -1;
	}

	if (tcgetattr(fd,&oldtio) == -1) {
		perror("tcgetattr");
		close_port_file(fd);
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = linklayer.baudRate | CS8 | CLOCAL | CREAD;
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
						i = CTRL;
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

	if (flag == SENDER) {
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

	if (flag == SENDER)
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
	if (flag == SENDER) {
		(void) signal(SIGALRM, alarmListener);
		alarm(linklayer.timeout);
		alarmFlag = 1;
		alarmCount = 0;
	}

	unsigned char addr = 0;
	unsigned char ctrl = 0;

	int i = START_FLAG;
	STOP = FALSE;

	while (STOP == FALSE) {
		unsigned char c = 0;

		if (flag == SENDER) {
			if (alarmCount >= linklayer.numTransmissions) {
				printf("EXCEDED NUMBER OF TRIES\n");
				close_port_file(fd);
				return -1;
			} else if (alarmFlag == 0) {
				send_disc(fd, flag);
				alarmFlag = 1;
				alarm(linklayer.timeout);
			}
		}

		if (read(fd, &c, 1)) {
			switch (i) {
				case START_FLAG:
					if (c == FLAG)
						i = ADDR;
					break;
				case ADDR:
					if ((flag == SENDER && c == ADDR_REC_RESP) || (flag == RECEIVER && c == ADDR_TRANS)) {
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
	alarm(linklayer.timeout);

	alarmFlag = 1;
	alarmCount = 0;

	unsigned int addr = 0;
	unsigned int ctrl = 0;

	int i = START_FLAG;
	STOP = FALSE;

	while (STOP == FALSE) {
		unsigned char c = 0;

		if (alarmCount >= linklayer.numTransmissions) {
			printf("EXCEDED NUMBER OF TRIES!\n");
			close_port_file(fd);
			return -1;
		} else if (alarmFlag == 0) {
			if (flag == SENDER) {
				send_set(fd);
			} else if (flag == RECEIVER) {
				send_disc(fd, flag);
			}

			alarmFlag = 1;
			alarm(linklayer.timeout);
		}

		if (read(fd, &c, 1)) {
			switch (i) {
				case START_FLAG:
					if (c == FLAG)
						i = ADDR;
					break;
				case ADDR:
					if ((flag == SENDER && c == ADDR_REC_RESP) || (flag == RECEIVER && c == ADDR_TRANS_RESP)) {
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

int llwrite(int fd, unsigned char * buffer, unsigned int length) {
	if (linklayer.flag == RECEIVER) {
		printf("RECEIVER CANNOT WRITE TO SERIAL PORT\n");
		return -1;
	}

	if (!initialized) {
		printf("SERIAL PORT ISNT INITIALIZED\n");
		return -1;
	}

	unsigned char bcc2 = 0;
	generate_bcc2(buffer, length, &bcc2);

	unsigned char aux_buffer[MAX_SIZE] = "";
	memcpy(aux_buffer, buffer, length);
	aux_buffer[length] = bcc2;

	unsigned char * stuffed_buffer = malloc(MAX_SIZE);
	int stuffed_length = 0;

	if ((stuffed_length = byteStuffing(aux_buffer, length+1, stuffed_buffer)) == -1) {
		printf("BYTE STUFFING ERROR\n");
		return -1;
	}

	int nbytes = 0;
	nbytes = send_data(fd, stuffed_buffer, stuffed_length);

	if (rec_resp_receiver(fd, stuffed_buffer, stuffed_length, bcc2) == -1) {
		printf("\n");
		return -1;
	}

	free(stuffed_buffer);
	linklayer.sequenceNumber = NEXT_INDEX(linklayer.sequenceNumber);

	return nbytes;
}

int generate_bcc2(unsigned char * buffer, unsigned int length, unsigned char * bcc2) {
	*bcc2 = 0;
	unsigned int i;

	for (i=0 ; i<length ; i++) {
		*bcc2 ^= buffer[i];
	}

	return 0;
}

int byteStuffing(unsigned char * buffer, unsigned int length, unsigned char * new_buffer) {
	unsigned int buff_pos = 0;
	unsigned int i;

	for (i=0 ; i<length ; i++) {
		char c = buffer[i];

		if (c == FLAG || c == OCTET_ESCAPE) {
			new_buffer[buff_pos++] = OCTET_ESCAPE;
			new_buffer[buff_pos++] = c ^ OCTET_DIFF;
		} else {
			new_buffer[buff_pos++] = c;
		}
	}

	return 0;
}

int send_data(int fd, unsigned char * buffer, unsigned int length) {
	unsigned char flag = FLAG;
	unsigned char addr = ADDR_TRANS;
	unsigned char ctrl = NEXT_CTRL_INDEX(linklayer.sequenceNumber);
	unsigned char bcc1 = addr ^ ctrl;

	write(fd, &flag, 1);
	write(fd, &addr, 1);
	write(fd, &ctrl, 1);
	write(fd, &bcc1, 1);
	write(fd, &buffer, length);
	write(fd, &flag, 1);

	return (length+5);
}


int rec_resp_receiver(int fd, unsigned char * buffer, unsigned int length, unsigned char bcc2) {
	(void) signal(SIGALRM, alarmListener);
	alarm(linklayer.timeout);

	alarmFlag = 1;
	alarmCount = 0;

	unsigned char addr = 0;
	unsigned char ctrl = 0;

	int i = START_FLAG;
	STOP = FALSE;

	while (STOP == FALSE) {
		unsigned char c = 0;

		if (alarmCount >= linklayer.numTransmissions) {
			printf("EXCEDED NUMBER OF TRIES\n");
			close_port_file(fd);
			return -1;
		} else if (alarmFlag == 0) {
			send_data(fd, buffer, length);
			alarmFlag = 1;
			alarm(linklayer.timeout);
		}

		if (read(fd, &c, 1)) {
			switch (i) {
				case START_FLAG:
					if (c == FLAG)
						i = ADDR;
					break;
				case ADDR:
					if (c == ADDR_REC_RESP) {
						addr = c;
						i = CTRL;
					} else if (c != FLAG) {
						i = START_FLAG;
					}
					break;
				case CTRL:
					if ((c == CTRL_REC_READY(NEXT_INDEX(linklayer.sequenceNumber))) || (c == CTRL_REC_READY(linklayer.sequenceNumber)) || (c == CTRL_REC_REJECT(linklayer.sequenceNumber))) {
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
						if (ctrl == CTRL_REC_READY(NEXT_INDEX(linklayer.sequenceNumber))) {
							alarm(0);
							STOP = TRUE;
						} else if (ctrl == CTRL_REC_REJECT(linklayer.sequenceNumber)) {
							send_data(fd, buffer, length);
							i = START_FLAG;
							alarm(linklayer.timeout);
						} else if (ctrl == CTRL_REC_READY(linklayer.sequenceNumber)) {
							alarm(0);
							STOP = TRUE;
						} else {
							i = ADDR;
						}
					} else {
						i = START_FLAG;
					}
					break;
			}
		}
	}

	return 0;
}

llread(int fd, unsigned char * buffer) {
	if (linklayer.flag == SENDER) {
		printf("SENDER CANNOT READ FROM SERIAL PORT");
		return -1;
	}

	if (!initialized) {
		printf("SERIAL PORT ISNT INITIALIZED\n");
		return -1;
	}

	return rec_data(fd, buffer);
}

int rec_data(int fd, unsigned char * buffer) {
	unsigned char addr = 0;
	unsigned char ctrl = 0;
	unsigned int dataCount = 0;
	unsigned char stuffed_buffer[MAX_SIZE];
	int i = START_FLAG;
	STOP = FALSE;

	while (STOP == FALSE) {
		unsigned char c = 0;

		if (read(fd, &c, 1)) {
			switch (i) {
				case START_FLAG:
					if (c == FLAG)
						i = ADDR;
					break;
				case ADDR:
					if (c == ADDR_TRANS) {
						addr = c;
						i = CTRL;
					} else if (c != FLAG) {
						i = START_FLAG;
					}
					break;
				case CTRL:
					if (c == NEXT_CTRL_INDEX(linklayer.sequenceNumber) || c == CTRL_DISC) {
						ctrl = c;
						i = BCC1;
					} else if (c == FLAG) {
						i = ADDR;
					} else {
						i = FLAG;
					}
					break;
				case BCC1:
					;
					int headerErrorProb = rand() % 100;

					if (headerErrorProb < linklayer.her) {
						i = START_FLAG;
					} else {
						if (c == (addr ^ ctrl)) {
							i = DATA;
						} else if (c == FLAG) {
							i = ADDR;
						} else {
							i = START_FLAG;
						}
					}
					break;
				case DATA:
					if (c != FLAG) {
						stuffed_buffer[dataCount] = c;
						dataCount++;
					} else {
						if (ctrl == CTRL_DISC) {
							if (send_disc(fd, RECEIVER))
								return -1;
							if (rec_ua(fd, RECEIVER))
								return -1;

							linklayer.openLink = 0;
							return -75;
						} else {
							unsigned int dataLength = 0;

							if ((dataLength = byteDestuffing(stuffed_buffer, dataCount, buffer)) == -1)
								return -1;

							unsigned char bcc2_received = buffer[dataLength - 1];
							unsigned char bcc2 = 0;

							generate_bcc2(buffer, dataLength - 1, &bcc2);

							int frameErrorProb = rand() % 100;

							if (frameErrorProb < linklayer.fer) {
								send_rej(fd);
								i = START_FLAG;
								dataCount = 0;
							} else {
								if (bcc2 == bcc2_received) {
									if (ctrl != NEXT_CTRL_INDEX(linklayer.sequenceNumber)) {
										send_rr(fd);
										i = START_FLAG;
										dataCount = 0;
									} else {
										linklayer.sequenceNumber = NEXT_INDEX(linklayer.sequenceNumber);
										send_rr(fd);

										return (dataLength - 1);
									}
								} else {
									if (ctrl != NEXT_CTRL_INDEX(linklayer.sequenceNumber))
										send_rr(fd);
									else
										send_rej(fd);

									i = START_FLAG;
									dataCount = 0;
								}
							}
						}
					}
					break;
			}
		}
	}

	return 0;
}

int byteDestuffing(unsigned char * buffer, unsigned int length, unsigned char * new_buffer) {
	unsigned int destuff_pos = 0;
	unsigned int i;

	for (i=0 ; i<length ; i++) {
		char c = buffer[i];

		if (c == OCTET_ESCAPE) {
			c == buffer[++i];

			if (c == (FLAG ^ OCTET_DIFF)) {
				new_buffer[destuff_pos++] = FLAG;
			} else if (OCTET_ESCAPE ^ OCTET_DIFF) {
				new_buffer[destuff_pos++] = OCTET_ESCAPE;
			} else {
				printf("DESTUFFING BUFFER ERROR\n");
				return -1;
			}
		} else {
			new_buffer[destuff_pos++] = c;
		}
	}

	return destuff_pos;
}

int send_rej(int fd) {
	unsigned char REJ[5];

	REJ[0] = FLAG;
	REJ[1] = ADDR_REC_RESP;
	REJ[2] = CTRL_REC_REJECT(linklayer.sequenceNumber);
	REJ[3] = REJ[1] ^ REJ[2];
	REJ[4] = FLAG;

	write(fd, REJ, 5);

	return 0;
}

int send_rr(int fd) {
	unsigned char RR[5];

	RR[0] = FLAG;
	RR[1] = ADDR_REC_RESP;
	RR[2] = CTRL_REC_READY(linklayer.sequenceNumber);
	RR[3] = RR[1] ^ RR[2];
	RR[4] = FLAG;

	write(fd, RR, 5);

	return 0;
}

int llclose(int fd, unsigned int flag) {
	if (!initialized) {
		printf("THIS IS ALREADY CLOSE\n");
		return -1;
	}

	if (flag == SENDER) {
		if (send_disc(fd, flag))
			return -1;
		if (rec_disc(fd, flag))
			return -1;
		if (send_ua(fd, flag))
			return -1;
	}

	if (close_port_file(fd))
		return -1;

	return 0;
}

int setBR(unsigned int baudrate) {
	linklayer.baudRate = baudrate;
	isDefaultBR = -1;

	return 0;
}

int setTO(unsigned int timeout) {
	linklayer.timeout = timeout;
	isDefaultTO = -1;

	return 0;
}

int setNT(unsigned int numtransmissions) {
	linklayer.numTransmissions = numtransmissions;
	isDefaultNT = -1;

	return 0;
}

int setHER(unsigned int her) {
	linklayer.her = her;
	isDefaultHER = -1;

	return 0;
}

int setFER(unsigned int fer) {
	linklayer.fer = fer;
	isDefaultFER = -1;

	return 0;
}
