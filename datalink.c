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
#include "alarme.h"

int ns=0;
int nr=0;

int llopen(int fd, ConnectionMode mode){

  int connected = 0, state=0, res=0, n_timeout=0;
  char SET[5], UA[5];
  char * frame;

  //Building frames
  buildConnectionFrame(SET,A_S,C_SET);
  buildConnectionFrame(UA,A_S,C_UA);

  switch (mode){
    case SEND:
       
       printf("ENTERING SENDER!\n");
        while(connected==0){

          switch(state){ // like a state machine to know if it is sending SET or waiting for UA
            
            case 0: //SENDS SET
              tcflush(fd,TCIOFLUSH); // clears port to making sure we are only sending SET

              if((res = write(fd,SET,5)) <5){
                perror("write():");
                return -1;
              }
              else printf("SET SENT\n");

              state =1;
              break;

            case 1: // GETTING UA

              printf("WAITING FOR UA\n");
              setAlarm(3);
              frame = NULL;
              while(frame == NULL){
                frame= connectionStateMachine(fd);

                if(timeout){  
                  n_timeout++;
                  if(n_timeout >= MaxTries){
                    stopAlarm();
                    printf("Nothing receveid for 3 times\n");
                    return -1;
                  }
                  else{ 
                    printf("WAITING FOR UA: Nothing was receveid after 3 seconds\n");
                    printf("Gonna try again!\n\n\n");
                    state=0; //tries to send again
                    timeout=0;
                    break;
                  }
                }
              }                  
              stopAlarm(); // something has been receveid by this point

              if( frame != NULL && UA[2]==frame[2] ){
                printf("Connection established!\n");
                connected=1;
              }
              else state=0;
              break;
          }
    }
    break;
          
  case RECEIVE:

          printf("ENTERING RECEIVER!\n");

          while(connected==0){

          switch(state){ // like a state machine to know if it is sending UA or waiting for SET
            
            case 0: //getting SET

                  printf("WAITING FOR SET\n");
                  frame= connectionStateMachine(fd);

                    if(frame!= NULL && SET[2]==frame[2]){
                      state=1;
                    }
                  break;
                  
            case 1: // sending UA

                  tcflush(fd,TCIOFLUSH); // clears port to making sure we are only sending UA

                  if((res = write(fd,UA,5)) <5){
                    perror("write():");
                    return -1;
                  }
                  printf("Connection Established!\n");
                  connected=1;
                  break;
            default:
                  break;
          }
    }
    break;
  }
  return 0;
}

int llclose(int fd, ConnectionMode mode){

  int connected = 0, state=0, res=0, n_timeout=0;
  char DISC[5], UA[5];
  char * frame;

  //Building frames
  buildConnectionFrame(UA,A_S,C_UA);
  buildConnectionFrame(DISC, A_S, C_DISC);

  conta=1;
  switch (mode){
  
    case SEND:
       printf("ENTERING SENDER!\n");
        while(connected==0){

          switch(state){ // like a state machine to know if it is sending DISC (or UA) or waiting for DISC
            
            case 0: //SENDS DISC
                  tcflush(fd,TCIOFLUSH); // clears port to making sure we are only sending SET

                  if((res = write(fd,DISC,5)) <5){
                    perror("write():");
                    return -1;
                  }
                  else printf("DISC SENT\n");

                  state =1;
                  break;
            case 1: // GETTING DISC

              printf("WAITING FOR DISC\n");
              setAlarm(3);
              frame = NULL;
              while (frame == NULL){                   
                frame = connectionStateMachine(fd);

                if( timeout ){
                  n_timeout++; 
                  if(n_timeout >= MaxTries){
                    stopAlarm();
                    printf("Nothing receveid for 3 times\n");
                    return -1;
                  }
                  else{ 
                    printf("WAITING FOR DISC: Nothing was receveid for 3 seconds\n");
                    printf("Gonna try again!\n\n\n");
                    state=0;
                    timeout=0;
                    break;
                  }
                }
              }
              stopAlarm();
              
              if(frame!= NULL && DISC[2]==frame[2] ){
                  printf("Connection terminated!\n");  
                  state=2;
              }
              else state=0;
              break;

            case 2:
                  tcflush(fd, TCIOFLUSH);  //clear port
					        // fprintf(stderr, "\nSending UA\n");
					        if((res = write(fd, UA, 5)) < 0) {  //0 ou 5?
					          perror("write()");
						        return -1;
					        }
					        printf("\nConnection terminated.\n");
					        connected = 1;
					        break;
          }
    }
    break;
          
  case RECEIVE:

          printf("ENTERING RECEIVER!\n");

          while(connected==0){

          switch(state){ // like a state machine to know if it is sending DISC
            
            case 0: //getting DISC

                  printf("WAITING FOR DISC\n");
                  frame= connectionStateMachine(fd);

                    if(DISC[2]==frame[2]){
                      state=1;
                    }
                  break;
                  
            case 1: // sending DISC back

                  tcflush(fd,TCIOFLUSH); // clears port to making sure we are only sending UA

                  if((res = write(fd,DISC,5)) <5){
                    perror("write():");
                    return -1;
                  }
                  else{
                    state=2;
                  }
                  break;
            
            case 2: //waiting for UA

                  printf("WAITING FOR UA\n");
                  frame= connectionStateMachine(fd);

                    if(UA[2]==frame[2]){
                      printf("\nConnection Terminated!\n");                      
                      connected=1;
                    }
                  break;
            default:
                  break;
          }
    }
    break;
  }
  return 0;
}

int llwrite(int fd, unsigned char* buffer,int length ){
  
  int transfering=1, res=0, frame_size=0, res1=0, done=1;
  unsigned char frame_to_send[SIZE_FRAME], frame_to_receive[SIZE_FRAME];
  unsigned char RR[5], REJ[5];
  
  if(ns==0){
    buildConnectionFrame(RR,A_S,C_RR0);
    buildConnectionFrame(REJ,A_S,C_REJ0);
  }
  else if (ns==1){
    buildConnectionFrame(RR,A_S,C_RR1);
    buildConnectionFrame(REJ,A_S,C_REJ1);
  }
  //construimos RR e REJ

	frame_size= buildFrame(frame_to_send, ns, buffer, length);

  while (transfering)
  { 
    //TIMEOUT CAUSION
    res = write(fd, frame_to_send, frame_size);
    setAlarm(3);
    done=1;
    while( readFromPort(fd, frame_to_receive) ) {
      if(timeout){
        n_timeout++;
        if(n_timeout >=MaxTries){
          stopAlarm();        
          printf("Nothing receveid for 3 times\n");
          return -1;
        }
      	else{
          printf("WAITING FOR WRITE ACKOLEGMENT: Nothing was receveid after 3 seconds\n");
          printf("Gonna try again!\n\n\n"); 
          timeout=0;
          done=0;
          break;
        }
      }
    }
    stopAlarm(); //something has been receveid by this point
  
    //if(res1==-1) printf("llwrite(): Couldn't read from port\n");
    
    if( memcmp(RR,frame_to_receive, 5) ){ //CHECK TO SEE IF RR
    	if(nr != ns )
      	ns=nr;
      	transfering=0;
    	/* else{
        	WHAT SHOULD WE DO IN THIS CASE MR.M?
      }*/
    }
    if(memcmp(REJ, frame_to_receive, 5) ){ //REJ CASE
      stopAlarm();
      done=0; //try again
    }
  }
  return res;
}

char* connectionStateMachine(int fd){

  connectionState currentState = START_CONNECTION;
  char c;
  static char message[5];
  int done = 0, i = 0;  

  while (!done){

    if (currentState == STOP_CON){
      done = 1;
    }
    else if (read(fd, &c, 1)== 0){
      return NULL;
    }
    
    switch(currentState){

      case START_CONNECTION:

        if(c == FLAG){
          message[i++] = c;
          currentState = FLAG_RCV;
        }
        break;

      case FLAG_RCV:
        if (c == A_R || c == A_S){
          message[i++] = c;
          currentState = A_RCV;
        }
        else if(c!=FLAG){
          i = 0;
          currentState = START_CONNECTION;
        }
        break;

      case A_RCV:

        if (c == C_SET || c== C_UA || c==C_DISC){
          message[i++] = c;
          currentState = C_RCV;
        }
        else if(c == FLAG){
          i = 1;
          currentState = FLAG;
        }
        else{
          i = 0;
          currentState = START_CONNECTION;
        }
        break;
      case C_RCV:

        if (c == (A_S^C_SET) || c== (A_S^C_UA) || c==(A_S^C_DISC) || c== (A_R^C_SET) || c== (A_R^C_UA) || c==(A_R^C_DISC)) {
          message[i++] = c;
          currentState = BCC_OK;
        }
        else if(c == FLAG){
          i = 1;
          currentState = FLAG_RCV;
        }
        else{
          i = 0;
          currentState = START_CONNECTION;
        }
        break;
      case BCC_OK:

        if (c == FLAG){
          message[i++] = c;
          currentState = STOP_CON;
        }
        else {
          i = 0;
          currentState = START_CONNECTION;
        }
        break;

      case STOP_CON: {
        message[i] = 0;
        done = 1;
        break;
      }
      default:
        break;
      }
  }
  return message;
}

