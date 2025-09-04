#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main (int argc, char **argv) {
	if (fork () == -1)
		printf ("fork failed\n");
	
	printf ("PID %d: Hello, fork.\n", getpid ());
}
