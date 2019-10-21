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
    char *file_name;
    int file_size;
}ApplicationLayer;

typedef enum {
    PARAM_FILE_SIZE, PARAM_FILE_NAME
} T_type;

extern ApplicationLayer Al; 

