uint64_t *semaphore;
uint64_t pid;

uint64_t main () {
    uint64_t *foo;
    uint64_t i;
    
	foo = "Hello World!    ";
    semaphore = malloc (sizeof (uint64_t));
    sem_init (semaphore, 1);

    pid = fork ();

    sem_wait (semaphore);
    
	while (*foo != 0) {
        write (1, foo, 8);
        foo = foo + 1;
    }
    
	sem_post (semaphore);
	
	// Evitar que el programa termine prematuramente
	if (pid != 0) {
	    // El padre espera un poco para que el hijo termine
	    i = 0;
	    while (i < 10000000) {
	        i = i + 1;
	    }
	}
}
