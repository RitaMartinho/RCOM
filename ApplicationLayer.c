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

ApplicationLayer Al;
ApplicationLayer Alr;
int receiver(int fd);

int main(int argc, char** argv){


    int fd=0, res=0;
    struct termios oldtio;

    int done = 0;
    char file[20];
    /* //unsigned char test[]="ol~a";   
      //unsigned char package[SIZE_DATAPACKAGE];
      //unsigned char  package2[SIZE_DATAPACKAGE];

      //size= buildDataPackage(test,package,4, &seq_n);
      //printf("size of package: %d\n", size);
    */  
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
          Al_setter();
      
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
       return 1;
      }
      close(Al.fd);

      break;
  
    case RECEIVE:
      receiver(fd);
      //close();
      break;
  }
  /*
    if(mode==0){
      res=llwrite(fd,package,size);

      if(res==-1) printf("llwrite didn't work\n");
    }

    if(mode==1){
        res= llread(fd,package2);

      if(res==-1){
        printf("llread didn't work\n");
        return 1;
      }
    }
 */
  if((res=llclose(fd, mode))==-1){
    printf("llclose not working \n");
  }

  if(resetPort(fd,&oldtio)<0){
    perror("resetPort():");
    exit(-1);
  }

  return 0;
}

//returns 0 in succes, -1 if error
int receiver(int fd){

  FILE *fd1;
  int res=0, done=0, state=0, name_size=0, output_file=0, its_data=0, size=0, L1=0, L2=0, c_value=0;
  unsigned char data_from_llread[SIZE_DATAPACKAGE];
  unsigned char package[SIZE_DATAPACKAGE-1];
  ControlPackage start[TLV_N], end[TLV_N];
  DataPackage data;

  if(res<0){
    perror("llopen(receive):");
    return -1;
  }

  while(!done){
    
    switch(state){

      case 0: //reading start packages and full Al struct
            
          
            res=llread(fd,data_from_llread);

            if(res<0){
              perror("llread()");
              return -1;
            }

            c_value=data_from_llread[0];
            
            for(int i=0; i<res-1;i++){ //[i+1] so it doesnt send the C -> it's not necessary at this point and we don't count it in our functions from tools

 
              package[i]=data_from_llread[i+1]; 
            }

            if(c_value==AP_START){
               printf(" right c %d\n", c_value);

              rebuildControlPackage(package,start);

              for(int i=0; i<TLV_N; i++){

                if(start[i].T==PARAM_FILE_SIZE){
                  const char *c = &start[i].V[0];
                  Al.file_size=atoi(c); 
                }

                if(start[i].T==PARAM_FILE_NAME){

                  name_size=(int)start[i].L;
                  Al.file_name=(char*)malloc(name_size);
                  strcpy(Al.file_name, (char*)start[i].V);
                }
              }
            }

            else if(c_value==AP_DATA){

              its_data=2;
              state=1;
              break;
            }
            else {
              printf(" wrong c %d\n", c_value);
              return -1;

            }

            output_file=open(Alr.file_name, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
            c_value=0;
            break;
      case 1:

          if(its_data==0){ // we do this if so it wont read again if we know already from case 1 that it is data

            res=llread(fd, data_from_llread);

            if(res<0){

              perror("llread()");
            }

            c_value=data_from_llread[0];


            its_data=1; // so it will start anyways in the next if
          }

          if(its_data==1){

             for(int i=0; i<res-1;i++){ //[i+1] so it doesnt send the C -> it's not necessary at this point and we don't count it in our functions from tools

              package[i]=data_from_llread[i+1]; 
            }
          }

          if(its_data==2 || its_data==1){
          if(c_value==AP_DATA){

            rebuildDataPackage(package,&data);
          }

          if(c_value==AP_END){
            
            printf("It's and end\n");
            state=2;
            break;
          }
          }

          
          res=write(output_file, data.file_data, 256*(int)data.L2+(int)data.L1);

          if(res<0){

            perror("write() to output file:");
            return -1;
          }

          memset(package, 0, SIZE_DATAPACKAGE); //because we are reusing it to read various (depends on the file) data_from_llread
      
          its_data=0; // so it can read more

          c_value=0;

          break;
      case 2:

          for(int i=0; i<res-1;i++){ //[i+1] so it doesnt send the C -> it's not necessary at this point and we don't count it in our functions from tools

              package[i]=data_from_llread[i+1]; 
          }

          rebuildControlPackage(package,end);

          
          for(int i=0; i<TLV_N; i++){

                if(end[i].T==PARAM_FILE_SIZE){

                  Al.file_size=1 /* contas maradas*/;
                }

                if(end[i].T==PARAM_FILE_NAME){

                  name_size=(int)end[i].L;
                  Al.file_name=(char*)malloc(name_size);
                  strcpy(Al.file_name, (char*)end[i].V);
                }
          }
          done=1;
          break;   
    }
  }
  
  if(close(output_file)<0){

    perror("close():");
    return 0;
  }  

  return 0;
}