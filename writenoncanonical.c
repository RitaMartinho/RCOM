/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A 0x03
#define C 0x03
 

volatile int STOP=FALSE;
void StateMachine(char);
int write_message(int);
int current=0;


int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
	
	if( !write_message(fd) ){
		printf("Error writting message\n"); 
	}

   

  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  */  
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}

int write_message(int fd){
	char buff[255];

/*	if (gets(buf)==NULL){
		
      perror("Reading for terminal failed");
      return -1;
	}*/
	int res = write(fd, 'a', sizeof(char));
	printf("Flag:%d\n", FLAG);
	return res;
}


void StateMachine(char c){


	switch(current){
		case 0:
			if(c == FLAG){
				current = 1;	
				printf("RECEIVED FLAG\n");
			}
			break;
		
		default:
			
			break;
	}
}
			/*case(FLAG_RCV){
			if(c == A){
				current=A_RCV;
			else if (c !=FLAG )
					current=START;	
			}
			break;		
		}
		case(A_RCV){
			if(c == C){
				current=C_RCV;	
			}
			else if(c == FLAG){
				current=FLAG_RCV;
			}
			else if(c != FLAG){
				current=START;
			}			
			break;
*/

