/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define F 0x7e
#define A 0x03
#define S 0x03

volatile int STOP=FALSE;

void send_trama(char* argv){
 int fd,res;
 unsigned char setup[5];
struct termios oldtio,newtio;

  setup[0] = F;
  setup[1] = A;
  setup[2] = S;
  setup[3] = setup[1]^setup[2];
  setup[4] = F;
 fd = open(argv, O_RDWR | O_NOCTTY );
    if (fd <0) { exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;

    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 30;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */


    tcflush(fd, TCIFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

printf("New termios structure set\n");


    res = write(fd,setup,5);
    printf("%d bytes written\n", res);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
  return res;
}

int main(int argc, char** argv)
{
  int tries = -1;
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    char aux[255];
    int state = 0;
   

    if ( (argc < 2) || 
         ((strcmp("/dev/ttyS0", argv[1])!=0) && 
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

   send_trama(argv[1]);
  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

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
/*                res = read(fd,buf,1);
                if(buf[0] == '\0') STOP=TRUE;
                printf("%x\n", buf[0]);*/
    res = read(fd,buf,1);
    aux[i] = buf[0];
    switch(state){
      case 0:{    
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
        else {
          state = 0;
          i=0;
        }
        break;
      }
      case 2:{
        if (aux[i] == F){
          state = 1;
          i=1;
        }
        else if(aux[i] == A) {
          state = 3;
          i++;
        }
        else {
          state = 0;
          i=0;
        } 
        break;  
      }
      case 3:{
        if (aux[i] == F){
          state = 1;
          i=1;
        }
        else if (aux[i] == (A^A)){
          state = 4;
          i++;
        }
        else{ 
          state = 0;
          i= 0;
        }
        break;
      }   
      case 4:{
        if(aux[i] == F){
          state = 5;  
          printf("TRAMA confirmed!\n");
        }
        else{
          state = 0;
          i= 0;
        }
        break;
      }
    }
    if(state==5)  STOP=TRUE;
    printf("%x\n", buf[0]);
        }
  
  if (res == 0 && tries!=3){
    tcsetattr(fd,TCSANOW,&oldtio);
          close(fd);
    tries++;
    send_trama(argv[1]);
  }
        tcsetattr(fd,TCSANOW,&oldtio);
        close(fd);


    return 0;
}
