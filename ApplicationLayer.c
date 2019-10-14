#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include "string.h"
#include "ApplicationLayer.h"
#include "tools.h"
#include "datalink.h"



int main(int argc, char** argv){

    int fd=0,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS4", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

  fd= setPort(argv[1], &oldtio);
  if(fd<0){ // set port configs

    perror("setPort():");
    exit(1);
  }

  //******************************************
  printf("Select Connection mode\n");
  printf("1: SEND   2:RECEIVE\n");
  ConnectionMode mode;
  int i; 
  scanf("%d",&i);
  mode =i-1;

  //printf("MODE: %d\n", mode);
  //*******************************************

  if((res=llopen(fd, mode))==-1){
    printf("llopen not working \n");
  }
  
  if((res=llclose(fd, mode))==-1){
    printf("llopen not working \n");
  }


  if(resetPort(fd,&oldtio)<0){

    perror("resetPort():");
    exit(-1);
  }

  return 0;
}