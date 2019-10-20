#pragma once //it only needs to be compiled once

#include <stdio.h>

typedef enum {
	SEND, RECEIVE
} ConnectionMode;

typedef struct 
{
    //file descriptor
    int fd; 
    // Type of connection (Sender or Receiver)
    ConnectionMode mode;
    //file to be transfered
    char *file;
    int file_size;
}ApplicationLayer;

extern ApplicationLayer Al; 