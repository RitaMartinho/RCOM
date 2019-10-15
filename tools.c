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


void buildConnectionFrame( char *connectionFrame, unsigned char A, unsigned char C){ // belongs to DATALINK

	connectionFrame[0] = FLAG;
	connectionFrame[1] = A;
	connectionFrame[2] = C;
	connectionFrame[3] = connectionFrame[1]^connectionFrame[2];
	connectionFrame[4] = FLAG;

} //supervisionFrame()


void buildFrame( char * frame, int C_ns, char* message, int lenght){ //belongs to DATALINK

	
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


char buildBBC2(char *message, int lenght){ //belongs to datalink

	char BCC2=0;

	for(int i=0; i< lenght; i++){

		BCC2 ^=message[i];
	}

	return BCC2;
}

//builds data package from file
int buildDataPackage(unsigned char* buffer, unsigned char* package, int size, int * seq_n){

	int aux=0, i=0;
	package[0]= AP_DATA; //C
	package[1]=(char)(*seq_n)++;

	if((*seq_n)== 256){ //module 255
		*seq_n=0;
	}

	aux= size %256; //
	package[2]=(size- aux)/256;
	package[3]=aux;

	for(i=0; i<size; i++){

		package[i+4]=buffer[i]; // data read from file into application package
	}

	return i+4; // returns size of package
}

//rebuild packet from datalink into DataPackage specific struct
void rebuildDataPackage(unsigned char* packet, DataPackage *packet_data){

	int i=0, j=0;
	int size_of_data=0;

	(*packet_data).N = packet[0];
	(*packet_data).L1= packet[1];
	(*packet_data).L2= packet[2];
	(*packet_data).file_data=(unsigned char*)malloc(256*(int)packet[1]+(int)packet[2]); //as shown in "guião-PDF"

	size_of_data= 256*(int)(*packet_data).L2+(int)(*packet_data).L1;

	for( i=3, j=0; j<size_of_data; i++, j++){

		(*packet_data).file_data[j]=packet[i]; //for each byte of data in packet, put in packet_data
	}

}


int buildControlPackage(unsigned char C, unsigned char* package, ControlPackage *tlv){


	int l=0, size=0;

	package[l++]=C; //control
	
	for(int i=0; i<TLV_N; i++){

		package[l++]= tlv[i].T;
		package[l++]= tlv[i].L;
		
		size=(int)tlv[i].L;

		for(int j=0; j< size; j++){

			package[l++] = tlv[i].V[j]; 
		}
	}

	return l; // returns lenght of control package created 
}

void rebuildControlPackage(unsigned char* package, ControlPackage *tlv){

	int i=0, size_v=0;
	for( int z=0; z> TLV_N; z++){

		tlv[z].T = package[i];
		i++;
		tlv[z].L= package[i];

		size_v=(int)(tlv[z].L);
		tlv[z].V= (unsigned char*)malloc(size_v); // would seg fault without this :')

		for(int j=0; j< size_v; j++){

			i++;
			tlv[z].V[j]= package[i];
		}
		i++; 
	}
}

int fileLenght(int fd){

	int lenght=0;

	if((lenght= lseek(fd,0,SEEK_END)<0)){ // goes 

		perror("lseek():");
		return -1;
	}

	if(lseek(fd,0, SEEK_SET)<0){

		perror("lseek()");
		return -1;
	}
	return lenght;
}


//returns lenght of frame read from port, -1 in error 
int readFromPort(int fd, unsigned char* frame){

    unsigned char tmp;
    int done=0, res=0, l=0;

    memset(frame, 0, 100000);

    while(!done){

        if((res=read(fd, &tmp,1))<0){
            perror("read() from port:");
            return -1;
        }

        if(tmp== FLAG){ // evaluate if end or start point

            if(l==0){ //start point 
                frame[l++]=tmp;

            }
            else{ // somewhere else in the middle, starts again

                if(frame[l-1] == FLAG){
                    memset(frame, 0, 100000);
                    l=0;
                    frame[l++]=FLAG;
                }
                else{ // in the end
                    frame[l++]= tmp;
                    done=1;
                }
            }
        }
        else{

            if(l>0){ // put in frame what reads in the middle
                frame[l++]=tmp;
            }
        }

    }
     
    return l;
}
