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
int receiver(int fd);

int main(int argc, char** argv){

    int fd=0, res=0;
    struct termios oldtio;
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
      break;
    
    case RECEIVE:
      printf("How do you wanna name the incoming file?\n");
      break;
  }

  int done = 0;
  char file[20];
	while (!done) {
		printf("\nFILENAME: ");

		if (scanf("%s", file) == 1){
            done = 1;}
		else
			printf("Invalid input. Try again:\n");
  }
  switch (mode)
  {
    case SEND:
      (Al.file_name) = file;
      printf("Al.file_name: %s\n", Al.file_name);
      break;
  
    case RECEIVE:
      (Al.file_name) = file;
      printf("Al.file_name: %s\n", Al.file_name);
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
     sender(fd);
      break;
  
    case RECEIVE:
      receiver(fd);
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

  int res=0, done=0, state=0, name_size=0, output_file=0, its_data=0;
  unsigned char data_from_llread[SIZE_DATAPACKAGE];
  unsigned char package[SIZE_DATAPACKAGE-1];
  ControlPackage start[TLV_N], end[TLV_N];
  DataPackage data;

  if(res<0){
    perror("llopen(receive):");
    return -1;
  }

int c_value=0;
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

        output_file=open(Al.file_name, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
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




  /*
  int z=0, u=0;
  while(!done){
    
    switch(state){

      case 0: //reading start packages and full Al struct
            
            res=llread(fd,data_from_llread);
            if(res<0){
              perror("llread()");
              return -1;
            }

            for(int i=0; i<res-1;i++){ //[i+1] so it doesnt send the C -> it's not necessary at this point and we don't count it in our functions from tools

              package[i]=data_from_llread[i+1]; 
            }

            if(data_from_llread[0]==AP_START){

              rebuildControlPackage(package,start);

              for(int i=0; i<TLV_N; i++){

                if(start[i].T==PARAM_FILE_SIZE){
                  const char *c = &start[i].V[0];
                  Al.file_size=atoi(c); 
                }

                if(start[i].T==PARAM_FILE_NAME){

                  name_size=(int)start[i].L;
                  //Al.file_name=(char*)malloc(name_size);
                  //strcpy(Al.file_name, (char*)start[i].V);
                }
              }
            }

            if(data_from_llread[0]==AP_DATA){
              
            printf("It's data nª%d\n", z++);
              its_data=1;
              state=1;
              break;
            }

            output_file=open(Al.file_name,O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
            memset(data_from_llread, 0, SIZE_DATAPACKAGE);
            
            break;
      case 1:

          if(its_data==0){ // we do this if so it wont read again if we know already from case 1 that it is data
            printf("It wasn't data nª%d\n", u++);
            
            res=llread(fd, data_from_llread);

            if(res<0){

              perror("llread()");
            }

            its_data=1; // so it will start anyways in the next if
          }

          if(its_data==1){
            
            for(int i=0; i<res-1;i++){ //[i+1] so it doesnt send the C -> it's not necessary at this point and we don't count it in our functions from tools

              package[i]=data_from_llread[i+1]; 
            }
          }

          if(data_from_llread[0]==AP_DATA){

            rebuildDataPackage(package,&data);
          }

          if(data_from_llread[0]==AP_END){

            state=2;
            break;
          }
          res=write(output_file, data.file_data, 256*(int)data.L2+(int)data.L1 );
          
          printf("Escrito NO FILE: %d\n", res);        
          if(res <0){

            perror("write() to output file:");
            return -1;
          }

          memset(data_from_llread, 0, SIZE_DATAPACKAGE); //because we are reusing it to read various (depends on the file) data_from_llread
      
          its_data=0; // so it can read more

          break;
      case 2:

          for(int i=0; i<res-1;i++){ //[i+1] so it doesnt send the C -> it's not necessary at this point and we don't count it in our functions from tools

              package[i]=data_from_llread[i+1]; 
          }

          rebuildControlPackage(package,end);

          
          for(int i=0; i<TLV_N; i++){

                if(end[i].T==PARAM_FILE_SIZE){

                  const char *c = &end[i].V[0];
                  Al.file_size=atoi(c); 
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
  */

  //MINE STATE MACHINE
  /*
  int c_value;
  while(!done){
    switch (state) {
      case 0:

        res=llread(fd,data_from_llread);
        if(res<0){
          perror("llread()");
          return -1;
        }

        for (int i = 0; i < res-1; i++){//i+1 cause we will already have C
          package[i]=data_from_llread[+1];
        }

        c_value=data_from_llread[0];
        if(c_value == AP_START || c_value==AP_END)
          state=1;
        else if(c_value==AP_DATA)
          state=2;
      break;
    
      case 1: //control
        if(c_value ==AP_START){
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
          output_file=open(Al.file_name, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
          state=0;
        }
        else if(c_value ==AP_END){
          rebuildControlPackage(package,end);

          for(int i=0; i<TLV_N; i++){
            if(end[i].T==PARAM_FILE_SIZE){
              const char *c = &end[i].V[0];
              Al.file_size=atoi(c); 
            }

            if(end[i].T==PARAM_FILE_NAME){

              name_size=(int)end[i].L;
             //Al.file_name=(char*)malloc(name_size);
              //strcpy(Al.file_name, (char*)end[i].V);
            }
          }
          done=1;
        }
      break;

      case 2:// data
        rebuildDataPackage(package, &data);
        
        res=write(output_file, data.file_data, 256*(int)data.L2+(int)data.L1);
        if(res<0){
          perror("write() to output file:");
          return -1;
        }
        memset(package, 0, SIZE_DATAPACKAGE); //because we are reusing it to read various (depends on the file) data_from_llread
        state=0;
      break;
    }
  }
  */
  if(close(output_file)<0){

    perror("close():");
    return 0;
  }  

  return 0;
}