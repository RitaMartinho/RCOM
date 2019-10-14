#ifndef DATALINK
#define TOOLS
#include "ApplicationLayer.h"

char* connectionStateMachine(int fd);
int llopen(int fd, ConnectionMode mode);
int llwrite(int fd);
int llclose(int fd, ConnectionMode mode);
void timeout();
#endif