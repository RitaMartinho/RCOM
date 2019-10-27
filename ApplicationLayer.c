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
#include "sender.h"
#include "receiver.h"

ApplicationLayer Al;
ApplicationLayer Alr;

int main(int argc, char** argv){

    int fd=0, res=0;
    struct termios oldtio;
    int done = 0, res_scan=0;
    char file[90];
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

  switch (mode)
  {
    case SEND:
      printf("What's the name of the file you wanna transfer?\n");
      while (!done) {
        printf("\nFILENAME: ");

          if (scanf("%s", file) == 1){
                  done = 1;}
          else
            printf("Invalid input. Try again:\n");

          (Al.file_name) = file;
          if(Al_setter()<0){

            printf("error setting Al\n");
            return -1;
          }
      }
      break;

    case RECEIVE:
      printf("How do you wanna name the incoming file?\n");

      	while (!done) {
		    printf("\nFILENAME: ");

		    if (scanf("%s", file) == 1){
            done = 1;}
		    else
			      printf("Invalid input. Try again:\n");
       }
         (Alr.file_name) = file;

      break;
  }

  if((res=llopen(fd, mode))==-1){
    printf("llopen not working \n");
    printf("Connection not possible, check cable and try again.\n");
    return 1;
  }

  switch (mode)
  {
    case SEND:
      if (sender(fd)<0 ){
       return -1;
      }
      close(Al.fd);

      break;

    case RECEIVE:
      if(receiver(fd, Alr)<0){
        return -1;
      }
      break;
  }

  if((res=llclose(fd, mode))==-1){
    printf("llclose not working \n");
  }

  if(resetPort(fd,&oldtio)<0){
    perror("resetPort():");
    exit(-1);
  }

  return 0;
}

