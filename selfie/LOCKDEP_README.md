# Sistema Lockdep - Detección de Deadlocks en Selfie

## Descripción

Se ha implementado un sistema de detección de deadlocks dinámico similar al **Lockdep** del kernel de Linux. Este sistema previene interbloqueos mediante el seguimiento del orden de adquisición de semáforos y locks (mutexes).

## Características Implementadas

### 1. **Clases de Bloqueo (Lock Classes)**
- Cada semáforo o lock recibe una clase única basada en su dirección de memoria
- Estructuras de datos para mantener hasta 128 clases de bloqueo (`MAX_LOCK_CLASSES`)
- Cada lock_class contiene:
  - `lock_addr`: Dirección virtual del lock/semáforo
  - `class_id`: Identificador único de la clase

### 2. **Grafo de Dependencias**
- Sistema de grafo dirigido para representar el orden de adquisición de locks
- Una arista A → B se crea cuando un proceso adquiere el lock B mientras ya posee el lock A
- Capacidad para hasta 512 dependencias (`MAX_LOCK_DEPENDENCIES`)
- Cada dependencia contiene:
  - `from_class`: Clase de lock origen
  - `to_class`: Clase de lock destino

### 3. **Detección de Ciclos**
- Algoritmo DFS (Depth-First Search) para detectar ciclos en el grafo
- Se ejecuta cada vez que un proceso intenta adquirir un nuevo lock
- Si se detecta un ciclo, el sistema:
  - Imprime un mensaje de advertencia detallado
  - Muestra la cadena completa de dependencias que causa el deadlock
  - Bloquea la adquisición del lock
  - Establece el código de salida a `EXITCODE_DEADLOCK` (28)

### 4. **Seguimiento por Contexto**
- Cada contexto (proceso) mantiene una lista de locks que posee actualmente
- Máximo 16 locks anidados por contexto (`MAX_HELD_LOCKS`)
- Al adquirir un lock:
  - Se verifica si crearía un ciclo
  - Se añade a la lista de held_locks del contexto
  - Se crean dependencias desde todos los locks que ya posee
- Al liberar un lock:
  - Se elimina de la lista de held_locks

## Archivos Modificados

### `selfie.c`
- **Líneas 2445-2520**: Estructuras de datos y declaraciones de Lockdep
- **Líneas 2412-2418**: Añadido soporte para held_locks en contextos
- **Líneas 2504-2506**: Variable global `lockdep_enabled`
- **Líneas 2648**: Añadido `EXITCODE_DEADLOCK = 28`
- **Líneas 12824-12833**: Inicialización de held_locks en `init_context()`
- **Líneas 13028-13331**: Implementación completa de funciones Lockdep
- **Líneas 8886-8943**: Integración en `implement_sem_wait()`
- **Líneas 8945-8987**: Integración en `implement_sem_post()`
- **Líneas 9076-9148**: Integración en `implement_lock_acquire()`
- **Líneas 9150-9207**: Integración en `implement_lock_release()`
- **Líneas 14361-14363**: Inicialización de Lockdep en boot

## Funciones Principales

### Inicialización
```c
void init_lockdep()
```
- Inicializa las estructuras de datos de lockdep
- Asigna memoria para lock_classes y lock_dependencies
- Llamada durante el arranque del sistema

### Manejo de Clases de Bloqueo
```c
uint64_t get_or_create_lock_class(uint64_t lock_addr)
uint64_t find_lock_class(uint64_t lock_addr)
```
- Busca o crea clases de bloqueo para cada lock/semáforo único
- Usa la dirección de memoria como identificador único

### Grafo de Dependencias
```c
void add_lock_dependency(uint64_t from_class, uint64_t to_class)
uint64_t dependency_exists(uint64_t from_class, uint64_t to_class)
```
- Añade aristas al grafo de dependencias
- Evita duplicados

### Detección de Ciclos
```c
uint64_t check_deadlock(uint64_t *context, uint64_t new_lock_class)
uint64_t detect_cycle_dfs(uint64_t start_class, uint64_t target_class, ...)
```
- Verifica si adquirir un nuevo lock crearía un ciclo
- Usa DFS para explorar el grafo

### Reporte de Errores
```c
void print_deadlock_warning(uint64_t *context, uint64_t new_lock_class, ...)
```
- Imprime información detallada cuando se detecta un deadlock:
  - ID del proceso
  - Dirección del lock que causaría el deadlock
  - Cadena completa de dependencias que forman el ciclo

### Manejo de Held Locks
```c
void add_held_lock(uint64_t *context, uint64_t lock_class)
void remove_held_lock(uint64_t *context, uint64_t lock_class)
```
- Mantiene la lista de locks que cada contexto posee actualmente

## Ejemplo de Uso

### Código que Causa Deadlock
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

### Salida Esperada
```
========================================
[LOCKDEP] DEADLOCK DETECTED!
========================================
Process ID: 0
Attempting to acquire lock at address: 0x...

Circular dependency chain:
  [0] Lock class 1 at address 0x...
     |
     v
  [1] Lock class 0 at address 0x...
     |
     v
  [2] Lock class 1 at address 0x...

This acquisition would create a cycle!
========================================

[LOCKDEP] BLOCKING LOCK ACQUISITION TO PREVENT DEADLOCK
```

## Archivos de Prueba

- `test-deadlock.c`: Test de deadlock con locks (mutexes)
- `test-deadlock-sem.c`: Test de deadlock con semáforos

## Control del Sistema

El sistema Lockdep puede ser habilitado/deshabilitado mediante la variable global:
```c
uint64_t lockdep_enabled = 1;  // 1 = activado, 0 = desactivado
```

## Limitaciones

1. **Máximos configurables**:
   - `MAX_LOCK_CLASSES = 128`: Número máximo de clases de bloqueo únicas
   - `MAX_LOCK_DEPENDENCIES = 512`: Número máximo de dependencias en el grafo
   - `MAX_HELD_LOCKS = 16`: Número máximo de locks anidados por contexto

2. **Overhead de rendimiento**: La verificación de ciclos se ejecuta en cada adquisición de lock

3. **Memoria**: Requiere memoria adicional para mantener el grafo de dependencias

## Basado en

Este sistema está inspirado en el mecanismo Lockdep del kernel de Linux, que se encuentra en `kernel/locking/lockdep.c`. La implementación adapta los conceptos principales al contexto de Selfie:

- Clases de bloqueo para identificar tipos únicos de locks
- Grafo dirigido de dependencias de adquisición
- Detección de ciclos mediante búsqueda en profundidad
- Reporte detallado de dependencias circulares

## Referencias

- Linux Kernel Lockdep: `kernel/locking/lockdep.c`
- Documentación de Lockdep: https://www.kernel.org/doc/html/latest/locking/lockdep-design.html
