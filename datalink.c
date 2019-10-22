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
  unsigned char SET[5], UA[5];
  char * frame;

  //Building frames
  buildConnectionFrame(SET,A_S,C_SET);
  buildConnectionFrame(UA,A_S,C_UA);
  timeout_counter = 1;
  switch (mode){
    case SEND:
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
                    printf("Nothing received for 3 times\n");
                    return -1;
                  }
                  else{ 
                    printf("Nothing was received after 3 seconds\n");
                    printf("Gonna try again!\n\n\n");
                    state=0; //tries to send again
                    timeout=0;
                    break;
                  }
                }
              }                  
              stopAlarm(); // something has been received by this point

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
        }
      }
    break;
  }
  return 0;
}

int llclose(int fd, ConnectionMode mode){

  int connected = 0, state=0, res=0, n_timeout=0;
  unsigned char DISC[5], UA[5];
  char * frame;

  //Building frames
  buildConnectionFrame(UA,A_S,C_UA);
  buildConnectionFrame(DISC, A_S, C_DISC);

  timeout_counter=1;
  switch (mode){
    case SEND:
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
                  if(n_timeout >= MaxTries){
                    stopAlarm();
                    printf("Nothing received for 3 times\n");
                    return -1;
                  }
                  else{ 
                    printf("WAITING FOR DISC: Nothing was received for 3 seconds\n");
                    printf("Gonna try again!\n\n\n");
                    state=0;
                    timeout=0;
                    break;
                  }
                }
              }
              
              if(frame!= NULL && DISC[2]==frame[2] ){ //GOT DISC
                stopAlarm();
                  state=2;
              }
              else state=0;
              break;

            case 2:
              tcflush(fd, TCIOFLUSH);  //clear port
              if((res = write(fd, UA, 5)) < 5) {  //0 ou 5?
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
      while(connected==0){

        switch(state){ // like a state machine to know if it is sending DISC
          
          case 0: //getting DISC

            printf("WAITING FOR DISC\n");
            frame= connectionStateMachine(fd);

              if(frame!=NULL && DISC[2]==frame[2]){
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
        }
      }
    break;
  }
  return 0;
  
}

int llwrite(int fd, unsigned char* buffer,int length ){
  
  int transfering=1, res=0, frame_size=0, done=1;
  unsigned char frame_to_send[SIZE_FRAME], frame_to_receive[SIZE_FRAME];
  unsigned char RR[5], REJ[5];
  //n_timeout=0;
  //BUILD RR and REJ for comparison
  if(ns==0){
    buildConnectionFrame(RR,A_S,C_RR1);
    buildConnectionFrame(REJ,A_S,C_REJ0);
  }
  else if (ns==1){
    buildConnectionFrame(RR,A_S,C_RR0);
    buildConnectionFrame(REJ,A_S,C_REJ1);
  }
  tcflush(fd, TCIOFLUSH);
	frame_size= buildFrame(frame_to_send, ns, buffer, length);

  while (transfering)
  { 
    //TIMEOUT CAUSION
    res = write(fd, frame_to_send, frame_size);
    setAlarm(3);
    done = readFromPort(fd, frame_to_receive);
    if(done!=0)
      stopAlarm();
    while(!done) {
      if(timeout){
        printf("n_timeout=%d\n", n_timeout);
        if(n_timeout >=MaxTries){
          stopAlarm();        
          printf("Nothing received for 3 times\n");
          return -1;
        }
      	else{
          printf("WAITING FOR WRITE ACKOLEGMENT: Nothing was received after 3 seconds\n");
          printf("Gonna try again!\n\n"); 
          timeout=0;
          done=1;
          break;
        }
      }
    }
    
    if( memcmp(RR,frame_to_receive, 5) == 0 ){ //CHECK TO SEE IF RR
    	/*
        if(nr != ns ){
          ns=nr;
          transfering=0;
        }
        else continue; //Ns and nr equal, send again
      */
      ns = 1 -ns;
      transfering=0;
    }
    if(memcmp(REJ, frame_to_receive, 5) ){ //REJ CASE
      continue;
    }
  }
  return res;
}

int llread(int fd, unsigned char* frame_to_AL ){

  int done=0, state=0, res=0, i=0, j=0;
  int destuffed_data_size = 0;
  unsigned char frame_from_port[SIZE_FRAME];
  unsigned char data_frame_destuffed[SIZE_FRAME];
  unsigned char RR[5], REJ[5];
  unsigned char BCC2 = 0x00;
  unsigned char BCC2aux = 0x00;
  /*
    //build RR and REJ
    if(nr==1){

      buildConnectionFrame(RR,A_S,C_RR1);
      buildConnectionFrame(REJ,A_S, C_REJ1);
    }else if(nr==0){

      buildConnectionFrame(RR, A_S,C_RR0);
      buildConnectionFrame(REJ,A_S,C_RR1);
    }
    //DISC
  */

  printf("entering llread\n");
  while(!done){

    switch(state){

      case 0://reads from port

        res=readFromPort(fd,frame_from_port);

        printf("read from port, %d\n", res);
        if(res==-1 || res== -2){
          
          return -1;
        }
  
        state=2;
        break;

      case 1: // IS THIS CASE POSSIBLE?

        if(frame_from_port[2]== C_NS0 && (nr==1)){


        }
        else if(frame_from_port[2] == C_NS1 && (nr==0)){


        }

        break;

      case 2: //check BCC1
        
        if((frame_from_port[1]^frame_from_port[2])!=frame_from_port[3]){ //wrong BCC1

          state=6;
        }
        else state =3;
        break;

      case 3://DESTUFFING
        destuffed_data_size = destuffing(res-1, frame_from_port, data_frame_destuffed); 
        state=4;
        break;
      case 4:  //check BCC2

        printf("des data dize : %d",destuffed_data_size);
        BCC2=data_frame_destuffed[destuffed_data_size-1];
        BCC2aux=data_frame_destuffed[0];

        for(int k=1; k<destuffed_data_size-1; k++){
          BCC2aux= BCC2aux ^ data_frame_destuffed[k];
        }

        printf("BCC2 : %d\n", BCC2);
        printf("BCC2aux: %d\n", BCC2aux);

        if(BCC2!=BCC2aux){
          state=6;
          break;
        }
        else state=5;
        break;
      case 5: 

          if(frame_from_port[2]== C_NS0 && nr==0){

            nr=1; // update nr
            buildConnectionFrame(RR,A_S,C_RR1);
          }
          else if(frame_from_port[2]== C_NS1 && nr==1){

            nr=0;// update nr
            buildConnectionFrame(RR, A_S,C_RR0);
          }
          //stuff well read, then send it to AppLayer
          for (i = 0, j = 0; i < destuffed_data_size-1; i++, j++) {
            frame_to_AL[j] = data_frame_destuffed[i];
        }

          //sends RR
          tcflush(fd,TCIOFLUSH);

          if( write(fd, RR, 5) < 5){
            perror(" Write() RR:");
            return -1;
          }

          printf("Leaving llread\n");
          done=1;
          break;
    case 6: //REJ case
        
        if(frame_from_port[2]== C_NS0 && nr==0){// frame 0, rej0
          buildConnectionFrame(REJ, A_S,C_REJ0); 
        }
        else if(frame_from_port[2]== C_NS1 && nr==1){// frame 1, rej1         
            buildConnectionFrame(REJ, A_S,C_REJ1);
        }
    
        tcflush(fd, TCIOFLUSH);       
        if( write( fd, REJ, 5)< 5){
          
          perror("Write () REJ:");
          return -1;
        }
        state=0; // trying again 
        break;
    }
  }
  return res-6;
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
    }
  }
  return message;
}

