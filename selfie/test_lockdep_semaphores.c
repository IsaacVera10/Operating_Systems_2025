// Test de deadlock con semáforos usando Lockdep
// Escenario: AB-BA deadlock con semáforos

uint64_t *sem_a;
uint64_t *sem_b;

uint64_t main() {
    // Inicializar semáforos
    sem_a = malloc(sizeof(uint64_t));
    sem_b = malloc(sizeof(uint64_t));
    
    sem_init(sem_a, 1);
    sem_init(sem_b, 1);
    
    // Primer orden: A -> B (OK)
    sem_wait(sem_a);
    sem_wait(sem_b);
    
    sem_post(sem_b);
    sem_post(sem_a);
    
    // Segundo orden: B -> A (debería detectar deadlock potencial)
    sem_wait(sem_b);
    sem_wait(sem_a);  // DEADLOCK: B ya está en held_locks, intentar A crea ciclo
    
    sem_post(sem_a);
    sem_post(sem_b);
    
    return 0;
}
