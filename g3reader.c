/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define F 0x7e
#define A 0x03
#define UA 0x07
volatile int STOP=FALSE;
int state = 0;

void listen_trama(char* arg){
 int fd,c, res, i=0;
    struct termios oldtio,newtio;
 char buf[255];
    char aux[255];

fd = open(arg, O_RDWR | O_NOCTTY );
    if (fd <0) {exit(-1); }

    tcgetattr(fd,&oldtio); /* save current port settings */

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 3;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);

    printf("New termios structure set\n");

//    while (STOP==FALSE) {       /* loop for input */
//    res = read(fd,buf,255);   /* returns after 5 chars have been input */
//      buf[res]=0;               /* so we can printf... */
//      printf(":%s:%d\n", buf, res);
//      if (buf[0]=='z') STOP=TRUE;
//   }

	while (STOP==FALSE) {
res = read(fd,buf,1);
aux[i] = buf[0];
switch(state){
case 0:
{		
if(aux[i] == F){
state = 1;
i++;
}
else{
state = 0;
}
break;
}
case 1:{
if(aux[i] == F){
state = 1;
}
else if(aux[i] == A){
state = 2;
i++;		
}
else {state = 0;
i=0;}
break;}

case 2:{
if (aux[i] == F){
state = 1;
i=1;}
else if(aux[i] == A) {
state = 3;
i++;
}
else {
state = 0;
i=0;}	
break;	}

case 3:{
if (aux[i] == F){
state = 1;
i=1;}
else if (aux[i] == (A^A)){
state = 4;
i++;}
else{ 
state = 0;
i= 0;}
break;}
case 4:{
if(aux[i] == F){
state == 5;	
printf("TRAMA confirmed!\n");
}
else{
state = 0;
i= 0;}
break;
}
}
if(state==5) STOP=TRUE;
printf("%x\n", buf[0]);

}
	printf("TEste");
	tcsetattr(fd,TCSANOW,&oldtio);
    	close(fd);
	return 0;
}

int main(int argc, char** argv)
{
    int fd,c, res, i=0;
    struct termios oldtio,newtio;
    char buf[255];
    char aux[255];
	unsigned char ua[5];

	ua[0] = F;
	ua[1] = A;
	ua[2] = UA;
	ua[3] = ua[1]^ua[2];
	ua[4] = F;

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

	//ESCUTA
    	listen_trama(argv[1]);

	sleep(2);

 	printf("Write to emissor\n");
	res = write(fd,ua, 5);
	printf("%d bytes written\n", res);


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);

    return 0;
}
