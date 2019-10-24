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


//sets Al struct with paramenters
void Al_setter(){
    int file_fd = open(Al.file_name, O_RDONLY);
    Al.fd=file_fd;
    (Al.file_size)=fileLenght(Al.fd);
    printf("FILE LENGTH : %d  \n\n", Al.file_size);
}
void tlv_setter(ControlPackage *tlv){
    //convert file_size to oct
    char fileSizeBuf[10];
    snprintf(fileSizeBuf, sizeof fileSizeBuf, "%d", Al.file_size);

    tlv[0].T=PARAM_FILE_SIZE;
    int i=(Al.file_size), count=0;
    while(i != NULL){
        i/=10;
        count++;
    }
    snprintf(&(tlv[0].L), count, "%d", count);
  
   // tlv[0].L=sizeof(Al.file_size); 
    tlv[0].V = (unsigned char *) malloc(sizeof(fileSizeBuf));
    for(int i=0;i < count; i++){
        tlv[0].V[i]=fileSizeBuf[i];
    }
    tlv[1].T=PARAM_FILE_NAME;
    tlv[1].L= strlen(Al.file_name);
    tlv[1].V = (unsigned char *) malloc(sizeof(tlv[1].L));
    for(int i = 0; i<strlen(Al.file_name); i++){
        tlv[1].V[i]=Al.file_name[i];
    }   
}

int sender(int fd){

    //convert file_size to oct
    char fileSizeBuf[10];
    snprintf(fileSizeBuf, sizeof fileSizeBuf, "%d", Al.file_size);

    ControlPackage tlv_start[2];
    tlv_setter(tlv_start);

    unsigned char Start_Controlpackage[10];
    int sizeControlPackage=buildControlPackage(AP_START, Start_Controlpackage, tlv_start);
    llwrite(fd, Start_Controlpackage,sizeControlPackage); 

    unsigned char fileBuf[SIZE_DATAPACKAGE-4]; //-4 cause of the data headers
    unsigned int bytesread=0;
    unsigned char DataPackage[SIZE_DATAPACKAGE];
    int i=0;

    int readsize, bytesleft, data_size, total_bytesread=0, byteswritten;
    while( total_bytesread < Al.file_size ) {       
        bytesleft=Al.file_size-total_bytesread;
        if(bytesleft > SIZE_DATAPACKAGE -4) {
            readsize = SIZE_DATAPACKAGE-4;
        } else {
            readsize =bytesleft;
        }
        total_bytesread += readsize;
        if((bytesread = read(Al.fd, fileBuf, readsize)) < 0) {
            printf("Error reading from file\n");
            return -1;
        }

        if((data_size = buildDataPackage(fileBuf, DataPackage, bytesread, (i++)%255)) < 0) {
            perror("buildDataPackage");
            return -1;
        }

        //printf("data package size: %d\n", data_size);
        byteswritten = llwrite(fd, DataPackage,data_size);

   //     printf("llwriten: %d\n", byteswritten);
        if(byteswritten < 0) {
            printf("Connection LOST, check cable and try again\n");
            return -1;
        } 

        memset(fileBuf, 0, SIZE_DATAPACKAGE);
        printProgressBar(total_bytesread, Al.file_size);
    }

    ControlPackage tlv_end[2];
    tlv_setter(tlv_end);
    
    unsigned char End_Controlpackage[SIZE_DATAPACKAGE-4];
    buildControlPackage(AP_END, End_Controlpackage, tlv_end);
    llwrite(fd, End_Controlpackage, SIZE_DATAPACKAGE); 
  
  return 1;
}