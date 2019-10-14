#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <signal.h>

#include "tools.h"

//returns -1 in error 
int setPort(char *port, struct termios *oldtio){

    if((strcmp("/dev/ttyS0", port) != 0) && (strcmp("/dev/ttyS4", port) != 0)) {
		perror("setPort(): wrong argument for port");
		return -1;
	}

	/*
		Open serial port device for reading and writing and not as controlling tty
		because we don't want to get killed if linenoise sends CTRL-C.
	*/
	struct termios newtio;

	int fd;
	if ((fd = open(port, O_RDWR | O_NOCTTY )) < 0) {
		perror(port);
		return -1;
	}

	if ( tcgetattr(fd, oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused (estava a 0)*/
	newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received (estava a 5)*/

	/*
		VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
		leitura do(s) proximo(s) caracter(es)
	*/

	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
		perror("tcsetattr");
		return -1;
	}

	printf("\nNew termios structure set\n");

	return fd;

}


//returns -1 in error
int resetPort(int fd, struct termios *oldtio) {

	if ( tcsetattr(fd, TCSANOW, oldtio) == -1) {  //volta a por a configuracao original
		perror("tcsetattr");
		return -1;
	}

	close(fd);

	return 0;

} 


void buildConnectionFrame( char *connectionFrame, unsigned char A, unsigned char C){

	connectionFrame[0] = FLAG;
	connectionFrame[1] = A;
	connectionFrame[2] = C;
	connectionFrame[3] = connectionFrame[1]^connectionFrame[2];
	connectionFrame[4] = FLAG;

} //supervisionFrame()


void buildFrame( char * frame, int C_ns, char* message, int lenght){

	
	frame[0]=FLAG;
	frame[1]=A_S;
	
	if(C_ns){
		frame[2]= C_NS1;
	}
	else frame[2]=C_NS0;

	frame[3]= frame[1]^frame[2]; // BBC1

	//stuffing

	frame[4+lenght]=buildBBC2(message, lenght);

	frame[5+lenght]=FLAG;
}


char buildBBC2(char *message, int lenght){

	char BCC2=0;

	for(int i=0; i< lenght; i++){

		BCC2 ^=message[i];
	}

	return BCC2;
}
