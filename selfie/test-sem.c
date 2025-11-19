uint64_t *semaphore;

uint64_t main () {
    semaphore = malloc (sizeof (uint64_t));
    sem_init (semaphore, 1);

    write(1, "Before fork\n", 12);
    
    fork ();

    write(1, "After fork\n", 11);

    sem_wait (semaphore);
    
    write(1, "Inside CS\n", 10);
    
    sem_post (semaphore);
    
    write(1, "After CS\n", 9);
}
