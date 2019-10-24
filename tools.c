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

void buildConnectionFrame( unsigned char *connectionFrame, unsigned char A, unsigned char C){ // belongs to DATALINK

	connectionFrame[0] = FLAG;
	connectionFrame[1] = A;
	connectionFrame[2] = C;
	connectionFrame[3] = connectionFrame[1]^connectionFrame[2];
	connectionFrame[4] = FLAG;

} //supervisionFrame()

int buildFrame( unsigned char * frame, int C_ns, unsigned char* message, int lenght){ //belongs to DATALINK

	int l=0;
	unsigned char BCC2;
	BCC2=buildBCC2(message, lenght);

	frame[l++]=FLAG;
	frame[l++]=A_S;
	
	if(C_ns){
		frame[l++]= C_NS1;
	}
	else frame[l++]=C_NS0;

	frame[l++]= frame[1]^frame[2]; // BBC1

	l=stuffing(lenght,message, frame, l, BCC2);

	frame[l]=FLAG;

	return l+1; //returns lenght of frame (counts the 0 position)
}

unsigned char buildBCC2(unsigned char *message, int lenght){ //belongs to datalink

	unsigned char BCC2=0;

	for(int i=0; i< lenght; i++){

		BCC2 ^=message[i];
	}

	return BCC2;
}

//builds data package from file
int buildDataPackage(unsigned char* buffer, unsigned char* package, int size, int seq_n){

	int i=0, aux=0;
	package[0]= AP_DATA; //C
	package[1]=(char)(seq_n);

	aux=size%256;
	package[2]=(size -aux)/256;
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
	(*packet_data).L2= packet[1];
	(*packet_data).L1= packet[2];
	(*packet_data).file_data=(unsigned char*)malloc(256*(int)packet[1]+(int)packet[2]); //as shown in "guião-PDF"

	size_of_data= 256*(int)(*packet_data).L2+(int)(*packet_data).L1;

	for( i=3, j=0; j<size_of_data; i++, j++){

		(*packet_data).file_data[j]=packet[i]; //for each byte of data in packet, put in packet_data
	}

}

int buildControlPackage(unsigned char C, unsigned char* package, ControlPackage *tlv){

	int l=0, size=0;
	package[l++]=C; //control
	
	for(int i=0; i<TLV_N ; i++){
		package[l++]= tlv[i].T;
		package[l++]= tlv[i].L;
		
		size=(int)tlv[i].L;

		for(int j=0; j< size && tlv[i].V!=NULL ; j++){
			package[l++] = tlv[i].V[j]; 
		}
	}
	return l; // returns lenght of control package created 
}

void rebuildControlPackage(unsigned char* package, ControlPackage *tlv){

	int i=0, size_v=0;
	for( int z=0; z< TLV_N; z++){

		tlv[z].T = package[i];
		i++;
		tlv[z].L=sizeof(package[i]);
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

	if((lenght= lseek(fd,0,SEEK_END))<0){ // goes 

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

    memset(frame, 0, SIZE_FRAME);

    while(!done){
		res=read(fd, &tmp,1);
        if(res==-1){
            perror("read() from port = -1");
            return -1;
        }
		else if(res==0){
			 return 0;
		}
        else if(tmp== FLAG){ // evaluate if end or start point

            if(l==0){ //start point 
                frame[l++]=tmp;
            }
            else{ // somewhere else in the middle, starts again

                if(frame[l-1] == FLAG){
					memset(frame, 0, SIZE_FRAME);
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

int stuffing (int length, unsigned char* buffer, unsigned char* frame, int frame_length, unsigned char BCC2){

	for(int i=0; i<length; i++){
		if(buffer[i]== FLAG){//a flag is in the middle of the data

			frame[frame_length++]=ESC;
			frame[frame_length++]=FLAG_PPP;			
		}
		else if(buffer[i]==ESC){

			frame[frame_length++]=ESC;
			frame[frame_length++]=ESC_PPP;
		}
		else frame[frame_length++]= buffer[i];
	}

	if(BCC2==FLAG){
		frame[frame_length++]=ESC;
		frame[frame_length++]=FLAG_PPP;
	}
	else if(BCC2==ESC){
		frame[frame_length++]= ESC;
		frame[frame_length++]=ESC_PPP;
	}
	else frame[frame_length++]=BCC2;
	return frame_length;
}

//HERE BUFFER = PRE-DESTUFFING AND FRAME = AFTER-DESTUFFING
int destuffing(int length, unsigned char* buffer, unsigned char* frame){

	int frame_length=0;
	for(int i = 4; i< length; i++){
		if( buffer[i] == ESC) { //remove the next one
			if(buffer[i+1] == FLAG_PPP){
				frame[frame_length++] = FLAG;
			}
			else if(buffer[i+1] == ESC_PPP){ //remove the next one
				frame[frame_length++] = ESC;
			}
			i++;
		}
		else frame[frame_length++]=buffer[i];
	}
	return frame_length;
}

void printProgressBar(float current, float total) {
	int bar_length = 51;
	float percentage = 100.0 * current / total;

	printf("\rCompleted: %6.2f%% [", percentage);

	int i;
	int pos = percentage * bar_length / 100.0;

	for (i = 0; i < bar_length; i++){
		if(i <= pos) 
			printf("=");
		else printf(" ");
	}
	printf("]");

	fflush(stdout);
}
