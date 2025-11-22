/*
 * Test de Deadlock Detection con Lockdep
 * 
 * Este programa demuestra cómo Lockdep detecta deadlocks potenciales
 * en Selfie OS.
 * 
 * Compilar con:
 *   ./selfie -c test_lockdep.c -m 128
 */

uint64_t *lockA;
uint64_t *lockB;
uint64_t *lockC;

void test_simple_deadlock()
{
    // Test 1: Deadlock clásico AB-BA

    lockA = malloc(sizeof(uint64_t));
    lockB = malloc(sizeof(uint64_t));

    lock_init(lockA);
    lock_init(lockB);

    printf("\n=== Test 1: Simple AB-BA Deadlock ===\n");

    // Primera adquisición: A -> B (OK)
    printf("Step 1: Acquiring A then B...\n");
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_release(lockB);
    lock_release(lockA);
    printf("Step 1: OK\n\n");

    // Segunda adquisición: B -> A (DEADLOCK!)
    printf("Step 2: Acquiring B then A (should trigger deadlock warning)...\n");
    lock_acquire(lockB);
    lock_acquire(lockA);  // <- Lockdep debería detectar el ciclo aquí
    lock_release(lockA);
    lock_release(lockB);
}

void test_no_deadlock()
{
    lockA = malloc(sizeof(uint64_t));
    lockB = malloc(sizeof(uint64_t));

    lock_init(lockA);
    lock_init(lockB);

    printf("\n=== Test 2: No Deadlock (Consistent Order) ===\n");

    // Primera adquisición: A -> B
    printf("Step 1: Acquiring A then B...\n");
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_release(lockB);
    lock_release(lockA);
    printf("Step 1: OK\n\n");

    // Segunda adquisición: A -> B (mismo orden, OK)
    printf("Step 2: Acquiring A then B again...\n");
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_release(lockB);
    lock_release(lockA);
    printf("Step 2: OK\n\n");

    printf("Test 2 passed: No deadlock detected\n");
}

void test_chain_deadlock()
{
    lockA = malloc(sizeof(uint64_t));
    lockB = malloc(sizeof(uint64_t));
    lockC = malloc(sizeof(uint64_t));

    lock_init(lockA);
    lock_init(lockB);
    lock_init(lockC);

    printf("\n=== Test 3: Chain Deadlock (A->B->C->A) ===\n");

    // Crear cadena: A -> B -> C
    printf("Step 1: Building dependency chain A->B->C...\n");
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_acquire(lockC);
    lock_release(lockC);
    lock_release(lockB);
    lock_release(lockA);
    printf("Step 1: OK\n\n");

    // Intentar cerrar el ciclo: C -> A
    printf("Step 2: Attempting C->A (should detect cycle A->B->C->A)...\n");
    lock_acquire(lockC);
    lock_acquire(lockA);  // <- Debería detectar ciclo
    lock_release(lockA);
    lock_release(lockC);
}

void test_nested_locks()
{
    lockA = malloc(sizeof(uint64_t));
    lockB = malloc(sizeof(uint64_t));
    lockC = malloc(sizeof(uint64_t));

    lock_init(lockA);
    lock_init(lockB);
    lock_init(lockC);

    printf("\n=== Test 4: Nested Locks (Safe) ===\n");

    // Orden consistente: A -> B -> C
    printf("Acquiring A, B, C in order...\n");
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_acquire(lockC);

    printf("Releasing in reverse order...\n");
    lock_release(lockC);
    lock_release(lockB);
    lock_release(lockA);

    printf("Test 4 passed: Nested locks handled correctly\n");
}

void test_multiple_acquisitions()
{
    lockA = malloc(sizeof(uint64_t));

    lock_init(lockA);

    printf("\n=== Test 5: Multiple Acquisitions of Same Lock ===\n");

    printf("Acquiring lock A...\n");
    lock_acquire(lockA);
    printf("Releasing lock A...\n");
    lock_release(lockA);

    printf("Acquiring lock A again...\n");
    lock_acquire(lockA);
    printf("Releasing lock A again...\n");
    lock_release(lockA);

    printf("Test 5 passed: Same lock can be acquired multiple times (when released)\n");
}

uint64_t main()
{
    printf("\n");
    printf("==================================================\n");
    printf("LOCKDEP TEST SUITE\n");
    printf("==================================================\n");

    // Test 1: Deadlock simple AB-BA
    test_simple_deadlock();

    // Test 2: Sin deadlock (orden consistente)
    test_no_deadlock();

    // Test 3: Deadlock en cadena A->B->C->A
    test_chain_deadlock();

    // Test 4: Locks anidados (seguro)
    test_nested_locks();

    // Test 5: Múltiples adquisiciones del mismo lock
    test_multiple_acquisitions();

    printf("\n");
    printf("==================================================\n");
    printf("TEST SUITE COMPLETED\n");
    printf("==================================================\n");
    printf("\n");

    return 0;
}
