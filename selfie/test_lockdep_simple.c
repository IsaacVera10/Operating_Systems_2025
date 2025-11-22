/*
 * Test simple de Lockdep para Selfie OS
 * Demuestra detección de deadlock AB-BA
 */

uint64_t *lockA;
uint64_t *lockB;

uint64_t main() {
    lockA = malloc(8);
    lockB = malloc(8);
    
    lock_init(lockA);
    lock_init(lockB);
    
    // Primera adquisicion: A -> B (OK)
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_release(lockB);
    lock_release(lockA);
    
    // Segunda adquisicion: B -> A (DEADLOCK!)
    lock_acquire(lockB);
    lock_acquire(lockA);  // <- Lockdep deberia detectar el ciclo aqui
    lock_release(lockA);
    lock_release(lockB);
    
    return 0;
}
