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

int sender(int fd){

    int count_bytes2=0, ler=0, res=0, count_bytes=0, res1=0;
    unsigned char buffer[SIZE_DATAPACKAGE];
    //convert file_size to oct
    char fileSizeBuf[10];
    snprintf(fileSizeBuf, sizeof fileSizeBuf, "%d", Al.file_size);

    ControlPackage tlv_start[2];
    tlv_setter(tlv_start);

    unsigned char Start_Controlpackage[10];
    int sizeControlPackage=buildControlPackage(AP_START, Start_Controlpackage, tlv_start);
    llwrite(fd, Start_Controlpackage,sizeControlPackage); 

    unsigned char fileBuf[SIZE_DATAPACKAGE-4]; //-4 cause of the data headers
    unsigned int bytesread=1, DataPackageSize;
    unsigned char DataPackage[SIZE_DATAPACKAGE];
    int i=0;


    while(count_bytes2 <Al.file_size) {

					if(count_bytes2 + SIZE_DATAPACKAGE-4 <Al.file_size) {
						ler = SIZE_DATAPACKAGE-4;
						count_bytes2 += ler;
					} else {
						ler =Al.file_size - count_bytes2;
						count_bytes2 += ler;
					}

					if((res = read(Al.fd, buffer, ler)) < 0) {
						perror("read()");
						return -1;
					}

                    //printf("res: %d",res);

					if((res1 = buildDataPackage(buffer, DataPackage, res, &i)) < 0) {
						perror("buildDataPackage");
						return -1;
					}

                    printf("data package size: %d\n", res);
                    printf("C sent %d \n", DataPackage[0]);

                    
                    res = llwrite(fd, DataPackage,res1);

                    printf("llwriten: %d\n", res);
					if(res < 0) {
                        printf("Connection LOST, check cable and try again.\n");
						return -1;
					} else {
						count_bytes += res;
					}

                    memset(buffer, 0, SIZE_DATAPACKAGE);

    }
    /*while( bytesread> 0){

        printf("olaaa\n");
        bytesread=read(Al.fd, fileBuf, SIZE_DATAPACKAGE-4);

        printf("BYTESSSS: %d", bytesread);

        DataPackageSize = buildDataPackage(fileBuf, DataPackage, bytesread, (i++)%255);
        llwrite(fd, DataPackage, DataPackageSize);
        //empty the fileBuf 
        memset(fileBuf, 0, SIZE_DATAPACKAGE-4);
    }*/
    
    ControlPackage tlv_end[2];
    tlv_setter(tlv_end);
    
    unsigned char End_Controlpackage[SIZE_DATAPACKAGE-4];
    buildControlPackage(AP_END, End_Controlpackage, tlv_end);
    llwrite(fd, End_Controlpackage, SIZE_DATAPACKAGE); 
    return 1;
}