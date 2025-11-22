# 🔧 Guía de Integración de Lockdep en Selfie

## 📋 Pasos de Integración

### Paso 1: Actualizar CONTEXTENTRIES

**Ubicación**: `selfie.c`, línea ~2288

```c
// ANTES:
uint64_t CONTEXTENTRIES = 35;

// DESPUÉS:
uint64_t CONTEXTENTRIES = 37;  // Added held_locks_head and held_locks_count for Lockdep
```

### Paso 2: Insertar el Código de Lockdep

**Ubicación**: `selfie.c`, ANTES de la línea `// MICROKERNEL` (~línea 2450)

Copiar todo el contenido de `lockdep_code.c` en esa ubicación.

### Paso 3: Modificar `create_context()`

**Ubicación**: `selfie.c`, buscar función `create_context()`

Agregar al **final** de la función (antes del `return context;`):

```c
uint64_t *create_context(uint64_t *parent, uint64_t *vctxt) {
    // ... código existente ...

    // AGREGAR ESTO AL FINAL:
    set_held_locks_head(context, (uint64_t *)0);
    set_held_locks_count(context, 0);

    return context;
}
```

### Paso 4: Modificar `implement_lock_acquire()`

**Ubicación**: `selfie.c`, línea ~8950

**ANTES**:
```c
void implement_lock_acquire(uint64_t* context) {
    uint64_t lock_addr;
    uint64_t lock_id;
    uint64_t *lock;
    uint64_t sem_id;
    uint64_t *sem;
    uint64_t val;
    uint64_t n;
    uint64_t *waiters;
    uint64_t pid;

    lock_addr = *(get_regs(context) + REG_A0);
    lock_id = load_virtual_memory(get_pt(context), lock_addr);
    lock = used_locks + (lock_id * LOCKENTRIES);

    sem_id = get_lock_semaphore(lock);
    sem = used_semaphores + (sem_id * SEMAPHOREENTRIES);

    val = get_sem_value(sem);
    n = get_sem_n_waiters(sem);
    waiters = get_sem_waiters(sem);

    if (val > 0) {
        set_sem_value(sem, 0);
        set_lock_owner(lock, get_id_context(context));
        set_pc(context, get_pc(context) + INSTRUCTIONSIZE);
    } else {
        set_blocked(context, 1);

        if (n < 64) {
            if (waiters != (uint64_t*)0) {
                pid = get_id_context(context);
                *(waiters + n) = pid;
                set_sem_n_waiters(sem, n + 1);
            }
        }
    }
}
```

**DESPUÉS** (agregar declaraciones y llamada a lockdep):
```c
void implement_lock_acquire(uint64_t* context) {
    uint64_t lock_addr;
    uint64_t lock_id;
    uint64_t *lock;
    uint64_t sem_id;
    uint64_t *sem;
    uint64_t val;
    uint64_t n;
    uint64_t *waiters;
    uint64_t pid;
    uint64_t lock_class;      // NUEVO
    uint64_t can_acquire;     // NUEVO

    lock_addr = *(get_regs(context) + REG_A0);
    lock_id = load_virtual_memory(get_pt(context), lock_addr);
    lock = used_locks + (lock_id * LOCKENTRIES);

    // NUEVO: Verificar con lockdep ANTES de intentar adquirir
    lock_class = lock_addr;  // Usamos la dirección como clase única
    can_acquire = lockdep_lock_acquire(context, lock_class);

    if (can_acquire == 0) {
        // DEADLOCK DETECTADO - no permitir adquisición
        printf("LOCKDEP: Lock acquisition denied due to potential deadlock\n");
        set_pc(context, get_pc(context) + INSTRUCTIONSIZE);
        return;
    }
    // FIN NUEVO

    sem_id = get_lock_semaphore(lock);
    sem = used_semaphores + (sem_id * SEMAPHOREENTRIES);

    val = get_sem_value(sem);
    n = get_sem_n_waiters(sem);
    waiters = get_sem_waiters(sem);

    if (val > 0) {
        set_sem_value(sem, 0);
        set_lock_owner(lock, get_id_context(context));
        set_pc(context, get_pc(context) + INSTRUCTIONSIZE);
    } else {
        set_blocked(context, 1);

        if (n < 64) {
            if (waiters != (uint64_t*)0) {
                pid = get_id_context(context);
                *(waiters + n) = pid;
                set_sem_n_waiters(sem, n + 1);
            }
        }
    }
}
```

### Paso 5: Modificar `implement_lock_release()`

**Ubicación**: `selfie.c`, línea ~9000

**ANTES**:
```c
void implement_lock_release(uint64_t* context) {
    uint64_t lock_addr;
    uint64_t lock_id;
    uint64_t *lock;
    uint64_t sem_id;
    uint64_t *sem;
    uint64_t n;
    uint64_t *waiters;
    uint64_t waiter_pid;
    uint64_t *waiter_ctx;
    uint64_t i;

    lock_addr = *(get_regs(context) + REG_A0);
    lock_id = load_virtual_memory(get_pt(context), lock_addr);
    lock = used_locks + (lock_id * LOCKENTRIES);

    sem_id = get_lock_semaphore(lock);
    sem = used_semaphores + (sem_id * SEMAPHOREENTRIES);

    n = get_sem_n_waiters(sem);
    waiters = get_sem_waiters(sem);

    set_sem_value(sem, 1);

    if (n > 0) {
        waiter_pid = *waiters;

        i = 0;
        while (i < n - 1) {
            *(waiters + i) = *(waiters + i + 1);
            i = i + 1;
        }

        *(waiters + n - 1) = -1;
        set_sem_n_waiters(sem, n - 1);

        waiter_ctx = find_context_by_id(waiter_pid);
        if (waiter_ctx != (uint64_t*)0) {
            set_blocked(waiter_ctx, 0);
            set_lock_owner(lock, waiter_pid);
        }
    } else {
        set_lock_owner(lock, -1);
    }

    set_pc(context, get_pc(context) + INSTRUCTIONSIZE);
}
```

**DESPUÉS** (agregar notificación a lockdep):
```c
void implement_lock_release(uint64_t* context) {
    uint64_t lock_addr;
    uint64_t lock_id;
    uint64_t *lock;
    uint64_t sem_id;
    uint64_t *sem;
    uint64_t n;
    uint64_t *waiters;
    uint64_t waiter_pid;
    uint64_t *waiter_ctx;
    uint64_t i;
    uint64_t lock_class;      // NUEVO

    lock_addr = *(get_regs(context) + REG_A0);
    lock_id = load_virtual_memory(get_pt(context), lock_addr);
    lock = used_locks + (lock_id * LOCKENTRIES);

    sem_id = get_lock_semaphore(lock);
    sem = used_semaphores + (sem_id * SEMAPHOREENTRIES);

    n = get_sem_n_waiters(sem);
    waiters = get_sem_waiters(sem);

    set_sem_value(sem, 1);

    if (n > 0) {
        waiter_pid = *waiters;

        i = 0;
        while (i < n - 1) {
            *(waiters + i) = *(waiters + i + 1);
            i = i + 1;
        }

        *(waiters + n - 1) = -1;
        set_sem_n_waiters(sem, n - 1);

        waiter_ctx = find_context_by_id(waiter_pid);
        if (waiter_ctx != (uint64_t*)0) {
            set_blocked(waiter_ctx, 0);
            set_lock_owner(lock, waiter_pid);
        }
    } else {
        set_lock_owner(lock, -1);
    }

    // NUEVO: Notificar a lockdep después de liberar
    lock_class = lock_addr;
    lockdep_lock_release(context, lock_class);
    // FIN NUEVO

    set_pc(context, get_pc(context) + INSTRUCTIONSIZE);
}
```

### Paso 6: Inicializar Lockdep

**Ubicación**: `selfie.c`, buscar la función `reset_microkernel()` (línea ~2502)

Agregar la llamada a `init_lockdep()`:

```c
void reset_microkernel()
{
    current_context = (uint64_t *)0;

    // AGREGAR ESTO:
    init_lockdep();

    while (used_contexts != (uint64_t *)0)
        used_contexts = delete_context(used_contexts, used_contexts);
}
```

### Paso 7 (Opcional): Soporte para Semáforos

Si también quieres detección de deadlock para semáforos, modifica:

#### `implement_sem_wait()`

```c
void implement_sem_wait(uint64_t* context) {
    uint64_t sem_addr;
    uint64_t sem_id;
    uint64_t *sem;
    uint64_t val;
    uint64_t n;
    uint64_t *waiters;
    uint64_t pid;
    uint64_t sem_class;       // NUEVO
    uint64_t can_acquire;     // NUEVO

    sem_addr = *(get_regs(context) + REG_A0);
    sem_id = load_virtual_memory(get_pt(context), sem_addr);
    sem = used_semaphores + (sem_id * SEMAPHOREENTRIES);

    // NUEVO: Verificar con lockdep
    sem_class = sem_addr;
    can_acquire = lockdep_semaphore_acquire(context, sem_class);

    if (can_acquire == 0) {
        printf("LOCKDEP: Semaphore acquisition denied due to potential deadlock\n");
        set_pc(context, get_pc(context) + INSTRUCTIONSIZE);
        return;
    }
    // FIN NUEVO

    // ... resto del código ...
}
```

#### `implement_sem_post()`

```c
void implement_sem_post(uint64_t* context) {
    // ... código existente ...

    uint64_t sem_class;   // NUEVO

    // ... código de liberación ...

    // NUEVO: Notificar a lockdep
    sem_class = sem_addr;
    lockdep_semaphore_release(context, sem_class);
    // FIN NUEVO

    set_pc(context, get_pc(context) + INSTRUCTIONSIZE);
}
```

## 🏗️ Compilación

```bash
cd /home/isaac/UTEC/Operating_Systems_2025/selfie
make clean
make selfie
```

Si hay errores, revisar:
1. Que `CONTEXTENTRIES` sea 37
2. Que el código de lockdep esté copiado correctamente
3. Que todas las modificaciones a funciones estén bien integradas

## 🧪 Testing

### Test Básico

```bash
# Compilar el test
./selfie -c test_lockdep.c -o test_lockdep.m

# Ejecutar
./selfie -l test_lockdep.m -m 128
```

### Verificar Tests Existentes

```bash
# Asegurarse de que los tests existentes sigan funcionando
python3 grader/self.py semaphores
python3 grader/self.py semaphore-lock
python3 grader/self.py fork
```

Si algún test falla, es posible que:
1. La lógica de lockdep esté rechazando locks legítimos
2. Haya un bug en la detección de ciclos
3. La integración con `implement_lock_acquire()` esté incorrecta

## 🐛 Debugging

### Desactivar Lockdep Temporalmente

Si necesitas desactivar lockdep para debugging:

```c
// Al inicio de main() o donde quieras
lockdep_enabled = 0;  // Desactivar
lockdep_enabled = 1;  // Reactivar
```

### Ver Estado de Lockdep

Puedes agregar una función de debugging:

```c
void lockdep_print_stats() {
    printf("LOCKDEP Stats:\n");
    printf("  Total dependencies: %lu / %lu\n", 
           lockdep_total_dependencies, 
           MAX_LOCKDEP_DEPENDENCIES);
    printf("  Enabled: %lu\n", lockdep_enabled);
}
```

### Ver Held Locks de un Contexto

```c
void lockdep_debug_context(uint64_t *context) {
    printf("Context %lu:\n", get_id_context(context));
    printf("  Held locks count: %lu\n", get_held_locks_count(context));
    print_held_locks(context);
}
```

## ✅ Checklist Final

- [ ] `CONTEXTENTRIES = 37`
- [ ] Código de lockdep copiado a `selfie.c`
- [ ] `create_context()` inicializa campos lockdep
- [ ] `implement_lock_acquire()` llama a `lockdep_lock_acquire()`
- [ ] `implement_lock_release()` llama a `lockdep_lock_release()`
- [ ] `reset_microkernel()` llama a `init_lockdep()`
- [ ] Compilación exitosa: `make selfie`
- [ ] Test simple ejecuta correctamente
- [ ] Tests existentes siguen funcionando

## 🎯 Resultado Esperado

Cuando ejecutes `test_lockdep.c`, deberías ver:

```
LOCKDEP: Initialized (max_deps=512, max_held=16)

==================================================
LOCKDEP TEST SUITE
==================================================

=== Test 1: Simple AB-BA Deadlock ===
Step 1: Acquiring A then B...
Step 1: OK

Step 2: Acquiring B then A (should trigger deadlock warning)...

======================================================
LOCKDEP: DEADLOCK DETECTED!
======================================================
Context 1 attempting to acquire lock 0x...
while already holding lock 0x...

This would create a circular dependency:
  0x... -> 0x... (new)
  0x... -> ... -> 0x... (existing)

Currently held locks by context 1:
  [0] lock_class = 0x...

Dependency chain:
  [0] 0x... -> 0x...

*** LOCK ACQUISITION DENIED ***
======================================================

LOCKDEP: Lock acquisition denied due to potential deadlock
...
```

## 📚 Referencias

- **Documentación Completa**: `LOCKDEP_IMPLEMENTATION.md`
- **Código Fuente Lockdep**: `lockdep_code.c`
- **Test Suite**: `test_lockdep.c`
- **Linux Lockdep**: `lockdep.c` (referencia)
