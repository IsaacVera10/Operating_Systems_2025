uint64_t *semaphore;
uint64_t pid;

uint64_t main () {
    semaphore = malloc (sizeof (uint64_t));
    sem_init (semaphore, 1);

    pid = fork ();

    sem_wait (semaphore);
    
    if (pid == 0) {
        write (1, "CHILD in CS\n", 12);
    } else {
        write (1, "PARENT in CS\n", 13);
    }
    
    sem_post (semaphore);
}
