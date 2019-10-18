#ifndef DATALINK
#define DATALINK
#include "ApplicationLayer.h"

char* connectionStateMachine(int fd);
int llopen(int fd, ConnectionMode mode);
int llwrite(int fd, unsigned char* buffer,int length );
int llread(int fd, unsigned char* frame_to_AL );
int llclose(int fd, ConnectionMode mode);

#endif