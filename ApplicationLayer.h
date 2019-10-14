#pragma once //it only needs to be compiled once

#include <stdio.h>

typedef enum {
	SEND, RECEIVE
} ConnectionMode;

typedef struct ApplicationLayer
{
    //file descriptor
    int fd; 
    // Type of connection (Sender or Receiver)
    ConnectionMode mode;
}test;

