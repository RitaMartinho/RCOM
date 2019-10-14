#ifndef TOOLS
#define TOOLS

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

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

#define MaxTries 3
#define TIMEOUT 3
#define T_PROP 3


typedef struct {
  unsigned char C;
	unsigned char T;
	unsigned char L;
	unsigned char *V;
} ControlPackage;

typedef struct {

  unsigned char C;
	unsigned char N;
	unsigned char L1;
	unsigned char L2;
	unsigned char *file_data;
} DataPackage;

typedef enum {
  START_PROTOCOL, WAIT_CONNECTION_UA , WRITE_READY, WAIT_WRITE_UA, STOP  
} protocolState;

typedef enum {
	START_CONNECTION, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP_CON
} connectionState;

int setPort(char *port, struct termios *oldtio);

int resetPort(int fd, struct termios *oldtio);
void buildConnectionFrame( char *connectionFrame, unsigned char A, unsigned char C);

char buildBBC2(char *message, int lenght);
#endif
