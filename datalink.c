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

  int connected = 0, state=0, res=0;
  unsigned char SET[5], UA[5];
  char * frame;

  //Building frames
  buildConnectionFrame(SET,A_S,C_SET);
  buildConnectionFrame(UA,A_S,C_UA);
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

              if( frame != NULL && UA[2]==frame[2] ){
              stopAlarm(); // something has been received by this point
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
                  else printf("\nDISC SENT\n");

                  state =1;
                  break;
            case 1: // GETTING DISC

              printf("\nWAITING FOR DISC\n");
              setAlarm(3);
              frame = NULL;
              while (frame == NULL){
                frame = connectionStateMachine(fd);

                if( timeout ){
                  n_timeout++;
                  if(n_timeout >= MaxTries){
                    stopAlarm();
                    printf("Nothing received for 3 times\n");
                    return -1;
                  }
                  else{
                    printf("\nWAITING FOR DISC: Nothing was received for 3 seconds\n");
                    printf("Gonna try again!\n\n\n");
                    state=0;
                    timeout=0;
                    break;
                  }
                }
              }
              stopAlarm();

              if(frame!= NULL && DISC[2]==frame[2] ){ //GOT DISC
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

            printf("\nWAITING FOR DISC\n");
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

  int transfering=1, res=0, frame_size=0, done=0;
  unsigned char frame_to_send[SIZE_FRAME], frame_to_receive[SIZE_FRAME];
  unsigned char RR[5], REJ[5];

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
    done=0;
    while(!done) {
      done = readFromPort(fd, frame_to_receive);
      if(timeout){
        if(n_timeout >=MaxTries){
          stopAlarm();
          printf("Nothing received for 3 times\n");
          return -1;
        }
      	else{
          printf("WAITING FOR WRITE ACKOLEGMENT: Nothing was received after 3 seconds\n");
          printf("Gonna try again!\n\n\n");
          timeout=0;
          //done=0;
          break;
        }
      }
    }

    if( memcmp(RR,frame_to_receive, 5) == 0 ){ //CHECK TO SEE IF RR
    	
      stopAlarm(); //something has been received by this point
      ns = 1 -ns;
      transfering=0;
    }
    if(memcmp(REJ, frame_to_receive, 5)==0 ){ //REJ CASE
      continue;
    }
  }
  return res;
}

int llread(int fd, unsigned char* frame_to_AL ){

  int done=0, state=0, res=0, i=0, j=0, discard=0;
  int destuffed_data_size = 0;
  unsigned char frame_from_port[SIZE_FRAME];
  unsigned char data_frame_destuffed[SIZE_FRAME];
  unsigned char RR[5], REJ[5];
  unsigned char BCC2 = 0x00;
  unsigned char BCC2aux = 0x00;
  

  while(!done){

    switch(state){

      case 0://reads from port

        res=readFromPort(fd,frame_from_port);

        if(res==-1 || res== -2){

          return -1;
        }

        state=2;
        break;

      case 1: // IS THIS CASE POSSIBLE?

        if(frame_from_port[2]== C_NS0 && (nr==1)){

          discard =1;
          nr=0;
          state=5;
          
        }
        else if(frame_from_port[2] == C_NS1 && (nr==0)){

          discard =1;
          nr=1;
          state=5;
          
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

        BCC2=data_frame_destuffed[destuffed_data_size-1];
        BCC2aux=data_frame_destuffed[0];

        for(int k=1; k<destuffed_data_size-1; k++){
          BCC2aux= BCC2aux ^ data_frame_destuffed[k];
        }

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
          if(discard==0){

            //stuff well read, then send it to AppLayer
            for (i = 0, j = 0; i < destuffed_data_size-1; i++, j++) {
              frame_to_AL[j] = data_frame_destuffed[i];
            }
          }
          //sends RR
          tcflush(fd,TCIOFLUSH);

          if( write(fd, RR, 5) < 5){
            perror(" Write() RR:");
            return -1;
          }

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
