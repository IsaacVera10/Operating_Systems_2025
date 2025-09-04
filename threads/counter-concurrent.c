#include <stdio.h>
#include <pthread.h>

#define BIG 100000000L

int counter = 0;

void *increase_counter (void *args) {
	for (int i = 0; i < BIG; ++i) {
		counter++;
	}
}

int main () {
	pthread_t a, b;

	pthread_create (&a, NULL, increase_counter, NULL);
	pthread_create (&b, NULL, increase_counter, NULL);

	pthread_join (a, NULL);
	pthread_join (b, NULL);

	printf ("Counter: %d\n", counter);
	return 0;
}
