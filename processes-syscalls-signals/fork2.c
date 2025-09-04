#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

static int g = 5;

int main (int argc, char **argv) {
	pid_t ret;
	int loc = 9;

	switch ((ret = fork ())) {
		case -1:
			printf ("fork failed, aborting\n");
			break;

		case 0:
			//printf ("Child process, PID %d\n", getpid ());
			loc++;
			g--;
			printf ("loc=%d g=%d\n", loc, g);
			printf ("Child (%d) done, exiting ...\n", getpid ());
			//printf ("exit succesful");
			break;

		default:
		#if 1
			sleep (2);
		#endif
			printf ("Process, PID %d\n", getpid ());
			loc--;
			g++;
			printf ("loc=%d g=%d\n", loc, g);
			break;
	}

	//printf ("This process (%d) will exit now...\n", getpid ());
	exit (0);
}
