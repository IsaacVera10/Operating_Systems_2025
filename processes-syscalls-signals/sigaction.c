#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

static void myHandler (int signum) {
	printf ("In myHandler with argument %d\n", signum);
}

int main () {
	unsigned long i = 0;

	int iRet;
	struct sigaction sAction;
	sAction.sa_flags = 0;
	sAction.sa_handler = myHandler;

	sigemptyset (&sAction.sa_mask);
	iRet = sigaction (SIGINT, &sAction, NULL);

	assert (iRet == 0);

	while (1) {
		printf ("Loop, %lu \n", i++);
		sleep (1);
	}

	return 0; /* Never get here */
}
