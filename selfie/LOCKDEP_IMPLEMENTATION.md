# 🔒 Implementación de Deadlock Avoidance estilo Lockdep para Selfie OS

## 📋 Índice
1. [Diseño General](#diseño-general)
2. [Estructuras de Datos](#estructuras-de-datos)
3. [Pseudocódigo](#pseudocódigo)
4. [Código Completo en C* de Selfie](#código-completo)
5. [Integración](#integración)
6. [Testing](#testing)

---

## 🎨 Diseño General

### Conceptos Clave

El sistema implementa **detección de deadlock dinámico** mediante:

1. **Lock Classes**: Cada lock (semáforo/mutex) tiene una clase única identificada por su dirección
2. **Grafo de Dependencias**: Estructura global que modela relaciones `A → B` (se adquiere B mientras se posee A)
3. **Held Locks**: Lista de locks actualmente poseídos por cada proceso
4. **Detección de Ciclos**: Algoritmo DFS que detecta ciclos antes de agregar nuevas dependencias

### Flujo de Operación

```
lock_acquire(B)
    ↓
¿Proceso posee locks?
    ↓ SÍ
Para cada lock A poseído:
    ↓
¿Agregar A→B crea ciclo?
    ↓ SÍ
RECHAZAR + WARNING
    ↓ NO
Agregar A→B al grafo
    ↓
Adquirir B normalmente
```

---

## 🗂️ Estructuras de Datos

### 1. Lock Dependency (Arista del Grafo)

```c
// Representa una dependencia A → B
// +---+------------------+
// | 0 | lock_class_from  | Clase del lock origen (A)
// | 1 | lock_class_to    | Clase del lock destino (B)
// | 2 | next             | Siguiente dependencia en la lista
// +---+------------------+
```

**Tamaño**: 3 entries

**Uso**: Nodo de lista enlazada para representar aristas del grafo

### 2. Held Lock (Lock Poseído)

```c
// Lock actualmente poseído por un contexto
// +---+------------------+
// | 0 | lock_class       | Dirección/ID del lock
// | 1 | next             | Siguiente held_lock en la lista
// +---+------------------+
```

**Tamaño**: 2 entries

**Uso**: Lista de locks que un proceso posee actualmente

### 3. Context Extension (Extensión al Contexto)

Se agregan 2 campos al contexto existente:

```c
// Extensión en CONTEXTENTRIES (de 35 → 37)
// | 35 | held_locks_head  | Primer lock poseído por este contexto
// | 36 | held_locks_count | Cantidad de locks poseídos
```

### 4. Variables Globales

```c
uint64_t *lockdep_dependency_graph;    // Head de lista de dependencias
uint64_t  lockdep_total_dependencies;  // Contador de dependencias
uint64_t  lockdep_enabled;             // Flag para habilitar/deshabilitar
uint64_t  lockdep_max_dependencies;    // Límite máximo (512)
uint64_t  lockdep_max_held_locks;      // Máximo locks anidados (16)
```

---

## 📝 Pseudocódigo

### Algoritmo 1: Agregar Held Lock

```
FUNCTION add_held_lock(context, lock_class):
    count = get_held_locks_count(context)
    
    IF count >= MAX_HELD_LOCKS:
        PRINT "WARNING: Max held locks exceeded"
        RETURN
    
    // Crear nuevo nodo
    held_lock = allocate_held_lock()
    set_held_lock_class(held_lock, lock_class)
    
    // Insertar al inicio de la lista
    head = get_held_locks_head(context)
    set_held_lock_next(held_lock, head)
    set_held_locks_head(context, held_lock)
    set_held_locks_count(context, count + 1)
END FUNCTION
```

### Algoritmo 2: Remover Held Lock

```
FUNCTION remove_held_lock(context, lock_class):
    prev = NULL
    current = get_held_locks_head(context)
    
    WHILE current != NULL:
        IF get_held_lock_class(current) == lock_class:
            // Encontrado, remover
            IF prev == NULL:
                // Es el primero
                set_held_locks_head(context, get_held_lock_next(current))
            ELSE:
                // En medio/final
                set_held_lock_next(prev, get_held_lock_next(current))
            
            count = get_held_locks_count(context)
            set_held_locks_count(context, count - 1)
            RETURN
        
        prev = current
        current = get_held_lock_next(current)
END FUNCTION
```

### Algoritmo 3: Detección de Ciclos (DFS)

```
FUNCTION has_cycle_dfs(from, to, visited, depth):
    IF depth > MAX_HELD_LOCKS:
        RETURN TRUE  // Prevenir recursión infinita
    
    IF from == to:
        RETURN TRUE  // Ciclo detectado!
    
    // Marcar como visitado
    visited[depth] = from
    
    // Explorar dependencias salientes de 'from'
    dep = lockdep_dependency_graph
    WHILE dep != NULL:
        dep_from = get_dependency_from(dep)
        dep_to = get_dependency_to(dep)
        
        IF dep_from == from:
            // Evitar re-visitar
            already_visited = FALSE
            i = 0
            WHILE i < depth:
                IF visited[i] == dep_to:
                    already_visited = TRUE
                    BREAK
                i = i + 1
            
            IF NOT already_visited:
                IF has_cycle_dfs(dep_to, to, visited, depth + 1):
                    RETURN TRUE
        
        dep = get_dependency_next(dep)
    
    RETURN FALSE
END FUNCTION
```

### Algoritmo 4: Verificar y Agregar Dependencia

```
FUNCTION check_and_add_dependency(from, to):
    // 1. Verificar si ya existe
    dep = lockdep_dependency_graph
    WHILE dep != NULL:
        IF get_dependency_from(dep) == from AND
           get_dependency_to(dep) == to:
            RETURN TRUE  // Ya existe, OK
        dep = get_dependency_next(dep)
    
    // 2. Verificar ciclo
    visited = allocate_array(MAX_HELD_LOCKS)
    initialize_visited(visited, MAX_HELD_LOCKS)
    
    IF has_cycle_dfs(to, from, visited, 0):
        RETURN FALSE  // Ciclo detectado!
    
    // 3. Agregar nueva dependencia
    IF lockdep_total_dependencies >= MAX_DEPENDENCIES:
        PRINT "WARNING: Max dependencies reached"
        RETURN FALSE
    
    new_dep = allocate_dependency()
    set_dependency_from(new_dep, from)
    set_dependency_to(new_dep, to)
    set_dependency_next(new_dep, lockdep_dependency_graph)
    lockdep_dependency_graph = new_dep
    lockdep_total_dependencies = lockdep_total_dependencies + 1
    
    RETURN TRUE
END FUNCTION
```

### Algoritmo 5: Lock Acquire con Lockdep

```
FUNCTION lockdep_lock_acquire(context, lock_class):
    IF NOT lockdep_enabled:
        RETURN TRUE
    
    // Iterar sobre todos los locks actualmente poseídos
    held = get_held_locks_head(context)
    WHILE held != NULL:
        from_class = get_held_lock_class(held)
        
        // Intentar agregar dependencia from → lock_class
        IF NOT check_and_add_dependency(from_class, lock_class):
            // DEADLOCK DETECTADO
            print_deadlock_warning(context, from_class, lock_class)
            RETURN FALSE
        
        held = get_held_lock_next(held)
    
    // Todo OK, agregar a held_locks
    add_held_lock(context, lock_class)
    RETURN TRUE
END FUNCTION
```

### Algoritmo 6: Lock Release con Lockdep

```
FUNCTION lockdep_lock_release(context, lock_class):
    IF NOT lockdep_enabled:
        RETURN
    
    remove_held_lock(context, lock_class)
END FUNCTION
```

---

## 💻 Código Completo en C* de Selfie

### Parte 1: Constantes y Estructuras

```c
// ============================================
// LOCKDEP: Deadlock Avoidance System
// ============================================

// ----- CONSTANTES -----

uint64_t MAX_LOCKDEP_DEPENDENCIES = 512;
uint64_t MAX_LOCKDEP_HELD_LOCKS = 16;

uint64_t DEPENDENCY_ENTRIES = 3;
uint64_t HELD_LOCK_ENTRIES = 2;

// ----- VARIABLES GLOBALES -----

uint64_t *lockdep_dependency_graph = (uint64_t *)0;
uint64_t lockdep_total_dependencies = 0;
uint64_t lockdep_enabled = 1;

uint64_t *lockdep_visited_buffer = (uint64_t *)0;

// ----- GETTERS/SETTERS PARA DEPENDENCY -----

uint64_t get_dependency_from(uint64_t *dep) { 
    return *dep; 
}

uint64_t get_dependency_to(uint64_t *dep) { 
    return *(dep + 1); 
}

uint64_t *get_dependency_next(uint64_t *dep) { 
    return (uint64_t *)*(dep + 2); 
}

void set_dependency_from(uint64_t *dep, uint64_t from) { 
    *dep = from; 
}

void set_dependency_to(uint64_t *dep, uint64_t to) { 
    *(dep + 1) = to; 
}

void set_dependency_next(uint64_t *dep, uint64_t *next) { 
    *(dep + 2) = (uint64_t)next; 
}

uint64_t *allocate_dependency() {
    return smalloc(DEPENDENCY_ENTRIES * sizeof(uint64_t));
}

// ----- GETTERS/SETTERS PARA HELD_LOCK -----

uint64_t get_held_lock_class(uint64_t *held) { 
    return *held; 
}

uint64_t *get_held_lock_next(uint64_t *held) { 
    return (uint64_t *)*(held + 1); 
}

void set_held_lock_class(uint64_t *held, uint64_t lock_class) { 
    *held = lock_class; 
}

void set_held_lock_next(uint64_t *held, uint64_t *next) { 
    *(held + 1) = (uint64_t)next; 
}

uint64_t *allocate_held_lock() {
    return smalloc(HELD_LOCK_ENTRIES * sizeof(uint64_t));
}

// ----- EXTENSIÓN AL CONTEXTO -----
// NOTA: CONTEXTENTRIES debe aumentarse de 35 a 37

uint64_t held_locks_head(uint64_t *context) { 
    return (uint64_t)(context + 35); 
}

uint64_t held_locks_count(uint64_t *context) { 
    return (uint64_t)(context + 36); 
}

uint64_t *get_held_locks_head(uint64_t *context) { 
    return (uint64_t *)*(context + 35); 
}

uint64_t get_held_locks_count(uint64_t *context) { 
    return *(context + 36); 
}

void set_held_locks_head(uint64_t *context, uint64_t *head) { 
    *(context + 35) = (uint64_t)head; 
}

void set_held_locks_count(uint64_t *context, uint64_t count) { 
    *(context + 36) = count; 
}
```

### Parte 2: Gestión de Held Locks

```c
// ============================================
// HELD LOCKS MANAGEMENT
// ============================================

void add_held_lock(uint64_t *context, uint64_t lock_class) {
    uint64_t *held_lock;
    uint64_t *head;
    uint64_t count;

    count = get_held_locks_count(context);

    if (count >= MAX_LOCKDEP_HELD_LOCKS) {
        printf("LOCKDEP WARNING: Max held locks (%lu) exceeded by context %lu\n",
               MAX_LOCKDEP_HELD_LOCKS, get_id_context(context));
        return;
    }

    held_lock = allocate_held_lock();
    set_held_lock_class(held_lock, lock_class);

    head = get_held_locks_head(context);
    set_held_lock_next(held_lock, head);

    set_held_locks_head(context, held_lock);
    set_held_locks_count(context, count + 1);
}

void remove_held_lock(uint64_t *context, uint64_t lock_class) {
    uint64_t *prev;
    uint64_t *current;
    uint64_t *next;
    uint64_t count;

    prev = (uint64_t *)0;
    current = get_held_locks_head(context);

    while (current != (uint64_t *)0) {
        if (get_held_lock_class(current) == lock_class) {
            // Found it, remove from list
            if (prev == (uint64_t *)0) {
                // It's the head
                next = get_held_lock_next(current);
                set_held_locks_head(context, next);
            } else {
                // It's in the middle or end
                next = get_held_lock_next(current);
                set_held_lock_next(prev, next);
            }

            count = get_held_locks_count(context);
            set_held_locks_count(context, count - 1);
            return;
        }

        prev = current;
        current = get_held_lock_next(current);
    }
}

uint64_t is_lock_held(uint64_t *context, uint64_t lock_class) {
    uint64_t *current;

    current = get_held_locks_head(context);

    while (current != (uint64_t *)0) {
        if (get_held_lock_class(current) == lock_class) {
            return 1;
        }
        current = get_held_lock_next(current);
    }

    return 0;
}
```

### Parte 3: Gestión del Grafo de Dependencias

```c
// ============================================
// DEPENDENCY GRAPH MANAGEMENT
// ============================================

uint64_t dependency_exists(uint64_t from, uint64_t to) {
    uint64_t *dep;

    dep = lockdep_dependency_graph;

    while (dep != (uint64_t *)0) {
        if (get_dependency_from(dep) == from) {
            if (get_dependency_to(dep) == to) {
                return 1;
            }
        }
        dep = get_dependency_next(dep);
    }

    return 0;
}

void add_dependency(uint64_t from, uint64_t to) {
    uint64_t *new_dep;

    if (lockdep_total_dependencies >= MAX_LOCKDEP_DEPENDENCIES) {
        printf("LOCKDEP WARNING: Max dependencies (%lu) reached\n", 
               MAX_LOCKDEP_DEPENDENCIES);
        return;
    }

    new_dep = allocate_dependency();
    set_dependency_from(new_dep, from);
    set_dependency_to(new_dep, to);
    set_dependency_next(new_dep, lockdep_dependency_graph);

    lockdep_dependency_graph = new_dep;
    lockdep_total_dependencies = lockdep_total_dependencies + 1;
}
```

### Parte 4: Detección de Ciclos (DFS)

```c
// ============================================
// CYCLE DETECTION (DFS)
// ============================================

void lockdep_init_visited(uint64_t *visited, uint64_t size) {
    uint64_t i;

    i = 0;
    while (i < size) {
        *(visited + i) = 0;
        i = i + 1;
    }
}

uint64_t lockdep_is_visited(uint64_t *visited, uint64_t depth, uint64_t node) {
    uint64_t i;

    i = 0;
    while (i < depth) {
        if (*(visited + i) == node) {
            return 1;
        }
        i = i + 1;
    }

    return 0;
}

uint64_t has_cycle_dfs(uint64_t from, uint64_t to, 
                        uint64_t *visited, uint64_t depth) {
    uint64_t *dep;
    uint64_t dep_from;
    uint64_t dep_to;
    uint64_t already_visited;

    // Prevent infinite recursion
    if (depth > MAX_LOCKDEP_HELD_LOCKS) {
        return 1;
    }

    // Cycle detected!
    if (from == to) {
        return 1;
    }

    // Mark as visited
    *(visited + depth) = from;

    // Explore outgoing edges from 'from'
    dep = lockdep_dependency_graph;

    while (dep != (uint64_t *)0) {
        dep_from = get_dependency_from(dep);
        dep_to = get_dependency_to(dep);

        if (dep_from == from) {
            // Check if already visited
            already_visited = lockdep_is_visited(visited, depth, dep_to);

            if (already_visited == 0) {
                if (has_cycle_dfs(dep_to, to, visited, depth + 1)) {
                    return 1;
                }
            }
        }

        dep = get_dependency_next(dep);
    }

    return 0;
}

uint64_t would_create_cycle(uint64_t from, uint64_t to) {
    uint64_t *visited;
    uint64_t result;

    visited = lockdep_visited_buffer;
    lockdep_init_visited(visited, MAX_LOCKDEP_HELD_LOCKS);

    result = has_cycle_dfs(to, from, visited, 0);

    return result;
}
```

### Parte 5: Impresión de Warnings

```c
// ============================================
// DEADLOCK WARNING
// ============================================

void print_dependency_chain(uint64_t from, uint64_t to) {
    uint64_t *dep;
    uint64_t depth;

    printf("  Dependency chain:\n");

    dep = lockdep_dependency_graph;
    depth = 0;

    while (dep != (uint64_t *)0) {
        if (depth < 10) {
            printf("    [%lu] 0x%lX -> 0x%lX\n", 
                   depth,
                   get_dependency_from(dep),
                   get_dependency_to(dep));
            depth = depth + 1;
        }
        dep = get_dependency_next(dep);
    }
}

void print_held_locks(uint64_t *context) {
    uint64_t *held;
    uint64_t count;

    printf("  Currently held locks by context %lu:\n", 
           get_id_context(context));

    held = get_held_locks_head(context);
    count = 0;

    while (held != (uint64_t *)0) {
        printf("    [%lu] lock_class = 0x%lX\n", 
               count, 
               get_held_lock_class(held));
        held = get_held_lock_next(held);
        count = count + 1;
    }
}

void print_deadlock_warning(uint64_t *context, 
                             uint64_t from_class, 
                             uint64_t to_class) {
    printf("\n");
    printf("======================================================\n");
    printf("LOCKDEP: DEADLOCK DETECTED!\n");
    printf("======================================================\n");
    printf("Context %lu attempting to acquire lock 0x%lX\n",
           get_id_context(context), to_class);
    printf("while already holding lock 0x%lX\n", from_class);
    printf("\n");
    printf("This would create a circular dependency:\n");
    printf("  0x%lX -> 0x%lX (new)\n", from_class, to_class);
    printf("  0x%lX -> ... -> 0x%lX (existing)\n", to_class, from_class);
    printf("\n");

    print_held_locks(context);
    printf("\n");
    print_dependency_chain(from_class, to_class);

    printf("\n");
    printf("*** LOCK ACQUISITION DENIED ***\n");
    printf("======================================================\n");
    printf("\n");
}
```

### Parte 6: Funciones Principales de Lockdep

```c
// ============================================
// MAIN LOCKDEP FUNCTIONS
// ============================================

uint64_t lockdep_lock_acquire(uint64_t *context, uint64_t lock_class) {
    uint64_t *held;
    uint64_t from_class;

    if (lockdep_enabled == 0) {
        return 1;
    }

    // Iterate over all currently held locks
    held = get_held_locks_head(context);

    while (held != (uint64_t *)0) {
        from_class = get_held_lock_class(held);

        // Check if adding from → lock_class would create a cycle
        if (dependency_exists(from_class, lock_class) == 0) {
            // New dependency, check for cycles
            if (would_create_cycle(from_class, lock_class)) {
                // DEADLOCK DETECTED
                print_deadlock_warning(context, from_class, lock_class);
                return 0;
            }

            // Safe to add
            add_dependency(from_class, lock_class);
        }

        held = get_held_lock_next(held);
    }

    // All OK, add to held_locks
    add_held_lock(context, lock_class);
    return 1;
}

void lockdep_lock_release(uint64_t *context, uint64_t lock_class) {
    if (lockdep_enabled == 0) {
        return;
    }

    remove_held_lock(context, lock_class);
}

void lockdep_semaphore_acquire(uint64_t *context, uint64_t sem_class) {
    lockdep_lock_acquire(context, sem_class);
}

void lockdep_semaphore_release(uint64_t *context, uint64_t sem_class) {
    lockdep_lock_release(context, sem_class);
}
```

### Parte 7: Inicialización

```c
// ============================================
// INITIALIZATION
// ============================================

void init_lockdep() {
    lockdep_dependency_graph = (uint64_t *)0;
    lockdep_total_dependencies = 0;
    lockdep_enabled = 1;

    // Allocate visited buffer for DFS
    lockdep_visited_buffer = smalloc(MAX_LOCKDEP_HELD_LOCKS * sizeof(uint64_t));
    lockdep_init_visited(lockdep_visited_buffer, MAX_LOCKDEP_HELD_LOCKS);

    printf("LOCKDEP: Initialized (max_deps=%lu, max_held=%lu)\n",
           MAX_LOCKDEP_DEPENDENCIES, MAX_LOCKDEP_HELD_LOCKS);
}

void reset_lockdep() {
    lockdep_dependency_graph = (uint64_t *)0;
    lockdep_total_dependencies = 0;
    lockdep_enabled = 1;
}
```

---

## 🔧 Integración

### Paso 1: Actualizar CONTEXTENTRIES

En `selfie.c`, línea ~2288:

```c
// ANTES:
uint64_t CONTEXTENTRIES = 35;

// DESPUÉS:
uint64_t CONTEXTENTRIES = 37;  // Added held_locks_head and held_locks_count
```

### Paso 2: Insertar el Código Lockdep

Agregar todo el código de lockdep **ANTES** de la sección `// MICROKERNEL` (alrededor de la línea 2450).

### Paso 3: Modificar `create_context()`

Buscar la función `create_context()` y agregar al final:

```c
uint64_t *create_context(uint64_t *parent, uint64_t *vctxt) {
    // ... código existente ...

    // Initialize lockdep fields
    set_held_locks_head(context, (uint64_t *)0);
    set_held_locks_count(context, 0);

    return context;
}
```

### Paso 4: Modificar `implement_lock_acquire()`

En `implement_lock_acquire()` (línea ~8950), agregar **ANTES** de intentar adquirir:

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

    // NUEVO: Verificar con lockdep
    lock_class = lock_addr;  // Usamos la dirección como clase
    can_acquire = lockdep_lock_acquire(context, lock_class);

    if (can_acquire == 0) {
        // DEADLOCK DETECTADO - no permitir adquisición
        // Podemos hacer panic o simplemente no bloquear
        printf("LOCKDEP: Lock acquisition denied due to potential deadlock\n");
        set_pc(context, get_pc(context) + INSTRUCTIONSIZE);
        return;
    }

    sem_id = get_lock_semaphore(lock);
    sem = used_semaphores + (sem_id * SEMAPHOREENTRIES);

    // ... resto del código existente ...
}
```

### Paso 5: Modificar `implement_lock_release()`

En `implement_lock_release()` (línea ~9000), agregar **DESPUÉS** de liberar:

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

    // ... código existente para liberar ...

    // NUEVO: Notificar a lockdep
    lock_class = lock_addr;
    lockdep_lock_release(context, lock_class);

    set_pc(context, get_pc(context) + INSTRUCTIONSIZE);
}
```

### Paso 6: Agregar Inicialización

En `main()` o donde se inicialice el sistema, agregar:

```c
init_lockdep();
```

### Paso 7: (Opcional) Semáforos

Si también quieres detección para semáforos, modifica de manera similar:

- `implement_sem_wait()`: Agregar `lockdep_semaphore_acquire()`
- `implement_sem_post()`: Agregar `lockdep_semaphore_release()`

---

## 🧪 Testing

### Test 1: Deadlock Simple AB-BA

```c
uint64_t *lockA;
uint64_t *lockB;

uint64_t main() {
    lockA = malloc(sizeof(uint64_t));
    lockB = malloc(sizeof(uint64_t));
    
    lock_init(lockA);
    lock_init(lockB);
    
    // Primer orden: A -> B (OK)
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_release(lockB);
    lock_release(lockA);
    
    // Segundo orden: B -> A (DEADLOCK!)
    lock_acquire(lockB);
    lock_acquire(lockA);  // <- Lockdep detecta el ciclo aquí
    
    return 0;
}
```

**Salida Esperada**:
```
LOCKDEP: Initialized (max_deps=512, max_held=16)
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
```

### Test 2: Sin Deadlock

```c
uint64_t main() {
    uint64_t *lockA;
    uint64_t *lockB;
    
    lockA = malloc(sizeof(uint64_t));
    lockB = malloc(sizeof(uint64_t));
    
    lock_init(lockA);
    lock_init(lockB);
    
    // Orden consistente A -> B
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_release(lockB);
    lock_release(lockA);
    
    // Mismo orden A -> B (OK)
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_release(lockB);
    lock_release(lockA);
    
    return 0;
}
```

**Salida Esperada**: Sin warnings, ejecución normal.

### Test 3: Cadena de 3 Locks

```c
uint64_t main() {
    uint64_t *lockA;
    uint64_t *lockB;
    uint64_t *lockC;
    
    // ... init ...
    
    // A -> B -> C (OK)
    lock_acquire(lockA);
    lock_acquire(lockB);
    lock_acquire(lockC);
    lock_release(lockC);
    lock_release(lockB);
    lock_release(lockA);
    
    // C -> A (DEADLOCK porque existe C <- B <- A)
    lock_acquire(lockC);
    lock_acquire(lockA);  // <- Detecta ciclo A->B->C->A
    
    return 0;
}
```

### Verificar Tests Existentes

```bash
# Ejecutar tests existentes
cd /home/isaac/UTEC/Operating_Systems_2025/selfie
python3 grader/self.py semaphores
python3 grader/self.py semaphore-lock
python3 grader/self.py fork
```

---

## 📊 Limitaciones y Consideraciones

### Limitaciones

1. **Memoria**: Cada dependencia ocupa ~24 bytes (3 × uint64_t)
   - Max 512 deps = ~12 KB
   
2. **Rendimiento**: DFS se ejecuta en cada `lock_acquire()`
   - Complejidad: O(V + E) donde V = held_locks, E = dependencies
   - Máximo: O(16 + 512) = O(528) operaciones
   
3. **Lock Classes**: Usa direcciones de memoria como identificador
   - Puede tener colisiones si se reutiliza memoria
   - Solución: Usar `lock_id` en lugar de `lock_addr` si es más estable

### Mejoras Futuras

1. **Hash Table**: Usar tabla hash para búsqueda O(1) de dependencias
2. **Cache de Ciclos**: Cachear resultados de DFS para locks frecuentes
3. **Stack Traces**: Guardar call stack cuando se crea cada dependencia
4. **Estadísticas**: Contar locks más problemáticos

---

## 📚 Referencias

- **Linux Lockdep**: `kernel/locking/lockdep.c`
- **Documentación Lockdep**: https://www.kernel.org/doc/html/latest/locking/lockdep-design.html
- **Selfie Grammar**: `grammar.md`
- **Selfie Syscalls**: Líneas 1340-1360 en `selfie.c`

---

## ✅ Checklist de Implementación

- [ ] Agregar código de lockdep a `selfie.c`
- [ ] Actualizar `CONTEXTENTRIES` de 35 a 37
- [ ] Modificar `create_context()` para inicializar campos lockdep
- [ ] Modificar `implement_lock_acquire()` con verificación lockdep
- [ ] Modificar `implement_lock_release()` con notificación lockdep
- [ ] Agregar `init_lockdep()` en la inicialización del sistema
- [ ] Compilar: `make selfie`
- [ ] Probar con test deadlock simple
- [ ] Verificar tests existentes no se rompan
- [ ] (Opcional) Agregar soporte para semáforos

---

**Autor**: Implementación de Lockdep para Selfie OS  
**Fecha**: 2025  
**Versión**: 1.0  
**Compatible con**: Selfie C* (sin `#define`, sin `&&`, sin inicialización en declaración)
