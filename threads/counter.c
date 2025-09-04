#include <stdio.h>

#define BIG 100000000L

int counter = 0;

void increase_counter () {
	for (int i = 0; i < BIG; ++i) {
		counter++;
	}
}

int main () {
	increase_counter();
	increase_counter();

	printf ("Counter: %d\n", counter);
	return 0;
}
