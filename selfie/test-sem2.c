uint64_t *semaphore;
uint64_t pid;

uint64_t main () {
    uint64_t *foo;
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
	
	// Solo el padre debe esperar
	if (pid != 0) {
	    write(1, "Parent waiting\n", 15);
	    while (1) {
	        // Espera activa para no terminar
	    }
	}
	
	write(1, "Child done\n", 11);
}
