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


//returns 0 in succes, -1 if error
int receiver(int fd){

  int res=0, done=0, state=0, name_size=0, output_file=0, its_data=0;
  unsigned char data_from_llread[SIZE_DATAPACKAGE];
  unsigned char package[SIZE_DATAPACKAGE-1];
  ControlPackage start[TLV_N], end[TLV_N];
  DataPackage data;



  res=llopen(fd,RECEIVE);
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

            for(int i=0; i<res-1;i++){ //[i+1] so it doesnt send the C -> it's not necessary at this point and we don't count it in our functions from tools

              package[i]=data_from_llread[i+1]; 
            }

            if(data_from_llread[0]==AP_START){

              rebuildControlPackage(package,start);

              for(int i=0; i<TLV_N; i++){

                if(start[i].T==PARAM_FILE_SIZE){

                  Al.file_size=1 /* contas maradas*/;
                }

                if(start[i].T==PARAM_FILE_NAME){

                  name_size=(int)start[i].L;
                  Al.file_name=(char*)malloc(name_size);
                  strcpy(Al.file_name, (char*)start[i].V);
                }
              }
            }

            if(data_from_llread[0]==AP_DATA){

              its_data=1;
              state=1;
              break;
            }

            output_file=open(Al.file_name, O_CREAT | O_WRONLY, S_IWUSR |S_IRUSR |S_IXUSR );
            memset(data_from_llread, 0, SIZE_DATAPACKAGE);
            
            break;
    case 1:

          if(its_data==0){ // we do this if so it wont read again if we know already from case 1 that it is data

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

  res=llclose(fd,RECEIVE);
  if(res<0){

    perror("llclose():");
    return -1;
  }

  if(close(output_file)<0){

    perror("close():");
    return 0;
  }  

  return 0;
}