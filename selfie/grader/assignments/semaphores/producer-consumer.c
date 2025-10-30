uint64_t N;
uint64_t *full;
uint64_t *empty;

uint64_t main () {
	uint64_t i;
	uint64_t *msg;
	uint64_t *aux;
	aux = "Reached\n";

	N = 1;	// Buffer size, producers should not produce twice, same for consumers
	full = malloc (sizeof (uint64_t));
	empty = malloc (sizeof (uint64_t));

	sem_init (full, 0);
	sem_init (empty, N);

	fork ();
	fork ();

	if (fork ()) {	// Producer
		i = 5;
		msg = "Prod....";
		
		while (i > 0) {
			sem_wait (empty);

			write (1, msg, 8);

			sem_post (full);

			i = i - 1;
		}
	} else {		// Consumer
		i = 5;
		msg = "Cons....";

		while (i > 0) {
			sem_wait (full);

			write (1, msg, 8);

			sem_post (empty);
		
			i = i - 1;
		}
	}
}
