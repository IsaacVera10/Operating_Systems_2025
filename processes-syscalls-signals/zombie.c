#include <unistd.h>
#include <stdlib.h>

int main () {
	if (fork () == 0) {
		exit(0); // Exits but parent doesn't wait. Will appear as Zombie
	}

	sleep (20);
}
