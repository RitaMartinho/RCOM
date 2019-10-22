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
}
void tlv_setter(ControlPackage *tlv){
    //convert file_size to oct
    char fileSizeBuf[10];
    snprintf(fileSizeBuf, sizeof fileSizeBuf, "%d", Al.file_size);

    tlv[0].T=PARAM_FILE_SIZE;
    tlv[0].L=sizeof(Al.file_size); 
    tlv[0].V = (unsigned char *) malloc(sizeof(fileSizeBuf));
    for(int i=0;i < strlen(fileSizeBuf); i++){
        tlv[0].V[i]=fileSizeBuf[i];
    }
    tlv[1].T=PARAM_FILE_NAME;
    tlv[1].L= strlen(Al.file_name);
    tlv[1].V = (unsigned char *) malloc(sizeof(tlv[1].L));
    for(int i = 0; i<strlen(Al.file_name); i++){
        tlv[1].V[i]=Al.file_name[i];
    }   
}

void sender(int fd){
    //set Al parameters
    Al_setter();
    
    //convert file_size to oct
    char fileSizeBuf[10];
    snprintf(fileSizeBuf, sizeof fileSizeBuf, "%d", Al.file_size);

    ControlPackage tlv_start[2];
    tlv_setter(tlv_start);

    unsigned char Start_Controlpackage[10];
    int sizeControlPackage=buildControlPackage(AP_START, Start_Controlpackage, tlv_start);
    llwrite(fd, Start_Controlpackage,sizeControlPackage); 

    unsigned char fileBuf[SIZE_DATAPACKAGE-4]; //-4 cause of the data headers
    unsigned int bytesread, DataPackageSize;
    unsigned char DataPackage[SIZE_DATAPACKAGE];
    int i=0, n_pakage=0;
    while( (bytesread=read(Al.fd, fileBuf, SIZE_DATAPACKAGE-4)) > 0){
       // printf("Send packagenº %d\n", n_pakage++);
        DataPackageSize = buildDataPackage(fileBuf, DataPackage, bytesread, (i++)%255);
        llwrite(fd, DataPackage, DataPackageSize);
        //empty the fileBuf 
        memset(fileBuf, 0, SIZE_DATAPACKAGE-4);
    }

    //NOT MINE
    /*int count_bytes2=0, ler=0, res=0, count_bytes=0, res1=0;
    while(count_bytes2 <Al.file_size) {

        if(count_bytes2 + SIZE_DATAPACKAGE-4 <Al.file_size) {
            ler = SIZE_DATAPACKAGE-4;
            count_bytes2 += ler;
        } else {
            ler =Al.file_size - count_bytes2;
            count_bytes2 += ler;
        }

        if((res = read(Al.fd, fileBuf, ler)) < 0) {
            perror("read()");
            return;
        }

        //printf("res: %d",res);

        if((res1 = buildDataPackage(fileBuf, DataPackage, res, &i)) < 0) {
            perror("buildDataPackage");
            return;
        }

        printf("data package size: %d\n", res);
        printf("C sent %d \n", DataPackage[0]);

        
        res = llwrite(fd, DataPackage,res1);

        printf("llwriten: %d\n", res);
        if(res < 0) {
            perror("llwrite()");
            return;
        } else {
            count_bytes += res;
        }

        memset(fileBuf, 0, SIZE_DATAPACKAGE);

    }*/
    ControlPackage tlv_end[2];
    tlv_setter(tlv_end);
    
    unsigned char End_Controlpackage[SIZE_DATAPACKAGE-4];
    buildControlPackage(AP_END, End_Controlpackage, tlv_end);
    llwrite(fd, End_Controlpackage, SIZE_DATAPACKAGE); 
  
}