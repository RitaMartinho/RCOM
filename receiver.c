#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#include "ApplicationLayer.h"
#include "tools.h"
#include "datalink.h"


//returns 0 in succes, -1 if error
int receiver(int fd, ApplicationLayer Alr){

  int res=0, done=0, state=0, name_size=0, output_file=0, its_data=0, c_value=0;
  int bytes_written=0, total_bytes_written=0;
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

          rebuildControlPackage(package,start);

          for(int i=0; i<TLV_N; i++){

            if(start[i].T==PARAM_FILE_SIZE){
              Al.file_size=atoi(&start[i].V[0]);
            }
            if(start[i].T==PARAM_FILE_NAME){
              name_size=(int)start[i].L;
              Al.file_name=(char*)malloc(name_size);
              strcpy(Al.file_name, (char*)start[i].V);
            }
          }
          printf("TOTAL FILE SIZE: %d\n", Al.file_size);
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
          state=2;
          break;
        }
      }

      bytes_written=write(output_file, data.file_data, 256*(int)data.L2+(int)data.L1);
      if(bytes_written<0){
        perror("write() to output file:");
        return -1;
      }
      total_bytes_written+=bytes_written;
      printProgressBar(total_bytes_written, Al.file_size);

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
          Al.file_size= atoi(&end[i].V[0]);
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
    return -1;
  }

  return 0;
}

