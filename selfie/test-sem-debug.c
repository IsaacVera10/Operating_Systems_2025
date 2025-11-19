uint64_t *semaphore;
uint64_t pid;

uint64_t main () {
    semaphore = malloc (sizeof (uint64_t));
    sem_init (semaphore, 1);

    write(1, "Before fork\n", 12);
    pid = fork ();
    write(1, "After fork\n", 11);

    write(1, "Before wait\n", 12);
    sem_wait (semaphore);
    write(1, "After wait\n", 11);
    
    if (pid == 0) {
        write (1, "CHILD in CS\n", 12);
    } else {
        write (1, "PARENT in CS\n", 13);
    }
    
    write(1, "Before post\n", 12);
    sem_post (semaphore);
    write(1, "After post\n", 11);
}
