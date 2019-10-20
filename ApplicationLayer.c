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

ApplicationLayer Al;

int main(int argc, char** argv){

    int fd=0,res=0, size=0, size1=0, res1=0;
    struct termios oldtio;
    unsigned char test[]="gtaiu~~~ebg";  
    unsigned char test1[]="gtaiu~~~ibg"; 
    unsigned char package[SIZE_DATAPACKAGE];
    unsigned char  package2[SIZE_DATAPACKAGE];
    unsigned char  package3[SIZE_DATAPACKAGE];
    unsigned char  package4[SIZE_DATAPACKAGE];

    int seq_n=0;
    
    size= buildDataPackage(test,package,12, &seq_n);
    size1= buildDataPackage(test1,package3,12, &seq_n);


    printf("size of package: %d\n", size);
    
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

  /*switch (mode)
  {
    case SEND:
      printf("What's the name of the file you wanna transfer?\n");
      break;
    
    case RECEIVE:
      printf("How do you wanna name the incoming file?\n");
      break;
  }*/

  /*int done = 0;
  char file[20];
	while (!done) {
		printf("\nFILENAME: ");

		if (scanf("%s", file) == 1){
            done = 1;}
		else
			printf("Invalid input. Try again:\n");
  }
  
  (Al.file) = file;
  int file_fd = open(Al.file, O_RDONLY);
  (Al.file_size)=fileLenght(file_fd);

  printf("File lenght= %d\n", Al.file_size);



  */
 
  if((res=llopen(fd, mode))==-1){
    printf("llopen not working \n");
    printf("Connection not possible, check cable and try again.\n");
    return 1;
  }


  if(mode==0){
     res=llwrite(fd,package,size);
      if(res==-1) printf("llwrite didn't work\n");
      res1=llwrite(fd,package3, size1);
      if(res1==-1) printf("llwrite 1 didn't work\n");

  }

  if(mode==1){
      res= llread(fd,package2);

    if(res==-1){
      printf("llread didn't work\n");
      return 1;
    }

    res1=llread(fd,package4);

    if(res1==-1){
      printf("llread didn't work\n");
      return 1;
    }

      for(int i=4; i<res; i++){

        printf("%c", package2[i]);
      }

            printf("\n");

      for(int i=4; i<res1; i++){

        printf("%c", package4[i]);
      }
      printf("\n");

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

//int sender(int fd, );