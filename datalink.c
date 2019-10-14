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


int timeout_flag = 0;
int count=0;


void timeout(){
  printf("Time-out # %d\n", count+1);
	count++;
  if(count >= MaxTries){
    timeout_flag=1;
  }
}


int llopen(int fd, ConnectionMode mode){

  int connected = 0, tries=0, state=0, res=0, failed=0;
  char SET[5], UA[5];
  int receivedSM=0;
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
                  //(void) signal(SIGALRM, timeout);
                  frame= connectionStateMachine(fd);

                  if( receivedSM == TIMEOUT){
                    
                    if(failed < MaxTries){

                      printf("WAITING FOR UA: Nothing was receveid after 3 seconds");
                      failed++;
                      state=0; //tries to send again
                    }
                    else{ // number of tries exceed
                        printf("Nothing receveid for 3 times");
                        return -1;
                    }
                  }
                  else{
                
                    if(UA[2]==frame[2] ){

                      printf("Connection established!\n");
                      connected=1;
                    }
                    else state=0;

                    }
                    
                  break;
            default:
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

                    if(SET[2]==frame[2]){
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

  int connected = 0, tries=0, state=0, res=0, failed=0;
  char DISC[5], UA[5];
  int receivedSM;
  char * frame;

  //Building frames
  buildConnectionFrame(UA,A_S,C_UA);
  buildConnectionFrame(DISC, A_S, C_DISC);

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
                  //(void) signal(SIGALRM, timeout);
                  frame= connectionStateMachine(fd);

                  if( receivedSM == TIMEOUT){
                    
                    if(failed < MaxTries){

                      printf("WAITING FOR UA: Nothing was receveid after 3 seconds");
                      failed++;
                      state=0; //tries to send again
                    }
                    else{ // number of tries exceed
                        printf("Nothing receveid for 3 times");
                        return -1;
                    }
                  }
                  
                
                  else if(DISC[2]==frame[2] ){
                        state=2;
                    }
                    
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

            default:
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

int llwrite(int fd){


}



char* connectionStateMachine(int fd){

  connectionState currentState = START_CONNECTION;
  char c;
  static char message[5];
  int done = 0, i = 0, res=0;
  

  while (!done){

    if (currentState == STOP_CON){
      done = 1;
    }
    else read(fd, &c, 1);
    
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

