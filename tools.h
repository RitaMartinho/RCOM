#ifndef TOOLS
#define TOOLS

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define SIZE_DATAPACKAGE 65539 // size of data packages = 256*(2⁸-1)+(2⁸-1) + 4
#define SIZE_FRAME 131085//(SIZE_DATAPACKAGE+1)*2+5
#define TLV_N 3// name of file, size of file, falgs


//DATALINK LEVEL
#define FLAG 0x7E //0111 1110
#define FLAG_PPP 0x5E
#define ESC 0x7D
#define ESC_PPP 0x5D
#define A_S 0x03 //0000 0011
#define A_R 0x01 //0000 0001
#define C_SET 0x03 //0000 0011
#define C_UA 0x07 //0000 0111
#define C_DISC 0x0B //00001011
#define C_RR0  0x05  //0000 0101
#define C_REJ0 0x01 //0000 0001
#define C_RR1  0x85  //1000 0101
#define C_REJ1 0x81  //1000 0001
#define C_NS0 0x00 //0000 0000
#define C_NS1 0x40 //0100 0000

//APPLICATION LEVEL
#define AP_START 0x02 //0000 0010
#define AP_DATA 0x01 //0000 0001
#define AP_END 0x03 //0000 0011

#define MaxTries 3
#define T_PROP 3
 
extern int timeout;
extern int n_timeout;




typedef struct {
  //unsigned char C;
	unsigned char T;
	unsigned char L;
	unsigned char *V;
} ControlPackage;

typedef struct {

  //unsigned char C;
	unsigned char N;
	unsigned char L1;
	unsigned char L2;
	unsigned char *file_data;
} DataPackage;

typedef enum {
	START_CONNECTION, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP_CON
} connectionState;

int setPort(char *port, struct termios *oldtio);
int resetPort(int fd, struct termios *oldtio);

void buildConnectionFrame( unsigned char *connectionFrame, unsigned char A, unsigned char C);
int buildFrame( unsigned char * frame, int C_ns, unsigned char* message, int lenght);
unsigned char buildBCC2(unsigned char *message, int lenght);

int buildDataPackage(unsigned char* buffer, unsigned char* package, int size, int * seq_n);
void rebuildDataPackage(unsigned char* packet, DataPackage *packet_data);

int buildControlPackage(unsigned char C, unsigned char* package, ControlPackage *tlv);
void rebuildControlPackage(unsigned char* package, ControlPackage *tlv);

int fileLenght(int fd);
int readFromPort(int fd, unsigned char* frame);


int stuffing (int length, unsigned char* buffer, unsigned char* frame, int frame_length, unsigned char BCC2);
int stuffing (int length, unsigned char* buffer, unsigned char* frame, int frame_length, unsigned char BCC2);

#endif
