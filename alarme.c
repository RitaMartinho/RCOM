#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0


int timeout;
int n_timeout=0;
void handler()                   // handler alarme
{
	printf("alarme # %d\n", n_timeout+1);
	timeout=TRUE;
	n_timeout++;
}

void stopAlarm() {
	struct sigaction action = {.sa_handler = NULL, .sa_flags = 0};
	sigemptyset(&action.sa_mask);
   action.sa_flags=0;

   sigaction(SIGALRM, &action, NULL);
   timeout=FALSE;
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

