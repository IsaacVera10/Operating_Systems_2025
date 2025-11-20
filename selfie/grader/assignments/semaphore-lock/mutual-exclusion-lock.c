uint64_t *lock;

uint64_t main () {
    uint64_t *foo;
	foo = "Hello World!    ";
    lock = malloc (sizeof (uint64_t));
    lock_init (lock);

    fork ();

    lock_acquire (lock);
    while (*foo != 0) {
        write (1, foo, 8);
        foo = foo + 1;
    }
    lock_release (lock);
}
