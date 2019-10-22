#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

int timeout_counter=0;

int timeout;
int n_timeout;
void handler()                   // handler alarme
{
	printf("alarme # %d\n", timeout_counter);
   timeout_counter++;
	timeout=TRUE;
	n_timeout++;
}

void stopAlarm() {
	struct sigaction action = {.sa_handler = NULL, .sa_flags = 0};
	sigemptyset(&action.sa_mask);
   action.sa_flags=0;

   sigaction(SIGALRM, &action, NULL);
   timeout=FALSE;
   timeout_counter=0;
   n_timeout=0;
	alarm(0);
}

void setAlarm(int time){
   struct sigaction action;
   action.sa_handler= handler;
   sigemptyset(&action.sa_mask); //inicializes the signal set to empty
   action.sa_flags = 0;

   timeout=FALSE;
   sigaction(SIGALRM, &action, NULL);
   alarm(time);
}

