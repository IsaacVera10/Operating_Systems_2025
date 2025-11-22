# 🎨 Diseño Técnico del Sistema Lockdep para Selfie

## 🌟 Resumen Ejecutivo

Este documento describe el diseño e implementación de un sistema de **Deadlock Avoidance** para Selfie OS, inspirado en el mecanismo Lockdep del kernel de Linux.

### Características Principales

- ✅ Detección **proactiva** de deadlocks (antes de que ocurran)
- ✅ Compatible con **locks** y **semáforos**
- ✅ Respeta todas las restricciones del lenguaje C* de Selfie
- ✅ No rompe tests existentes
- ✅ Overhead mínimo en tiempo y memoria

---

## 🏗️ Arquitectura del Sistema

### Componentes

```
┌─────────────────────────────────────────────────────────────┐
│                    LOCKDEP SYSTEM                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────┐         ┌───────────────┐              │
│  │  Held Locks   │────────▶│  Dependency   │              │
│  │   Per Context │         │     Graph     │              │
│  └───────────────┘         └───────────────┘              │
│         │                          │                       │
│         │                          ▼                       │
│         │                   ┌─────────────┐               │
│         └──────────────────▶│ Cycle       │               │
│                             │ Detection   │               │
│                             │    (DFS)    │               │
│                             └─────────────┘               │
│                                    │                       │
│                                    ▼                       │
│                             ┌─────────────┐               │
│                             │  Warning    │               │
│                             │  System     │               │
│                             └─────────────┘               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 🧩 Estructuras de Datos

### 1. Lock Dependency (Arista del Grafo)

Representa una dependencia `A → B` en el grafo.

```c
struct lock_dependency {
    uint64_t from;        // Clase del lock origen (A)
    uint64_t to;          // Clase del lock destino (B)
    struct lock_dependency *next;  // Lista enlazada
};
```

**Propiedades:**
- Tamaño: 24 bytes (3 × uint64_t)
- Máximo: 512 dependencias
- Memoria total: ~12 KB

**Implementación en C* de Selfie:**
```c
uint64_t DEPENDENCY_ENTRIES = 3;

uint64_t get_dependency_from(uint64_t *dep) { return *dep; }
uint64_t get_dependency_to(uint64_t *dep) { return *(dep + 1); }
uint64_t *get_dependency_next(uint64_t *dep) { return (uint64_t *)*(dep + 2); }
```

### 2. Held Lock (Lock Poseído)

Representa un lock actualmente poseído por un contexto.

```c
struct held_lock {
    uint64_t lock_class;   // Identificador del lock
    struct held_lock *next;  // Lista enlazada
};
```

**Propiedades:**
- Tamaño: 16 bytes (2 × uint64_t)
- Máximo por contexto: 16 locks anidados
- Memoria por contexto: ~256 bytes

**Implementación en C* de Selfie:**
```c
uint64_t HELD_LOCK_ENTRIES = 2;

uint64_t get_held_lock_class(uint64_t *held) { return *held; }
uint64_t *get_held_lock_next(uint64_t *held) { return (uint64_t *)*(held + 1); }
```

### 3. Context Extension

Extensión del contexto existente para mantener la lista de locks poseídos.

```c
// Extensión de 35 → 37 entradas
struct context {
    // ... campos existentes (0-34) ...
    struct held_lock *held_locks_head;   // [35] Primera entrada
    uint64_t held_locks_count;           // [36] Contador
};
```

**Implementación:**
```c
uint64_t CONTEXTENTRIES = 37;  // Incrementado de 35

uint64_t *get_held_locks_head(uint64_t *context) { 
    return (uint64_t *)*(context + 35); 
}

uint64_t get_held_locks_count(uint64_t *context) { 
    return *(context + 36); 
}
```

---

## 🔄 Flujo de Operación

### Adquisición de Lock

```
┌─────────────────────────────────────────────────────────┐
│ lock_acquire(B)                                         │
└─────────────┬───────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│ lockdep_lock_acquire(context, B)                        │
│                                                         │
│ 1. Obtener lista de held_locks del contexto            │
│                                                         │
│ 2. Para cada lock A en held_locks:                     │
│    ┌──────────────────────────────────────────┐       │
│    │ ¿Dependencia A→B ya existe?              │       │
│    │    SÍ → Continuar                         │       │
│    │    NO → Verificar ciclos                  │       │
│    └──────┬───────────────────────────────────┘       │
│           │                                             │
│           ▼                                             │
│    ┌──────────────────────────────────────────┐       │
│    │ would_create_cycle(A, B)?                │       │
│    │    SÍ → RECHAZAR + WARNING               │       │
│    │    NO → Agregar A→B al grafo             │       │
│    └──────────────────────────────────────────┘       │
│                                                         │
│ 3. Agregar B a held_locks del contexto                 │
│                                                         │
│ 4. Retornar TRUE (permitir adquisición)               │
└─────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│ Adquirir lock B (código original de Selfie)            │
└─────────────────────────────────────────────────────────┘
```

### Liberación de Lock

```
┌─────────────────────────────────────────────────────────┐
│ lock_release(B)                                         │
└─────────────┬───────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│ Liberar lock B (código original de Selfie)             │
└─────────────┬───────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│ lockdep_lock_release(context, B)                        │
│                                                         │
│ 1. Remover B de held_locks del contexto                │
│                                                         │
│ 2. Decrementar held_locks_count                        │
└─────────────────────────────────────────────────────────┘
```

---

## 🔍 Algoritmo de Detección de Ciclos

### DFS (Depth-First Search)

El algoritmo utiliza búsqueda en profundidad para detectar ciclos en el grafo de dependencias.

```python
def has_cycle_dfs(from, to, visited, depth):
    # Caso base 1: Límite de profundidad
    if depth > MAX_HELD_LOCKS:
        return True  # Prevenir recursión infinita
    
    # Caso base 2: Ciclo encontrado
    if from == to:
        return True
    
    # Marcar nodo actual como visitado
    visited[depth] = from
    
    # Explorar aristas salientes
    for each dependency (from → next):
        if next not in visited[0:depth]:
            if has_cycle_dfs(next, to, visited, depth + 1):
                return True
    
    return False
```

**Complejidad:**
- Tiempo: O(V + E) donde V = held_locks, E = dependencies
- Espacio: O(V) para el array de visitados
- Peor caso: O(16 + 512) = O(528) operaciones

### Ejemplo de Detección

```
Grafo actual:
    A → B
    B → C

Intento agregar C → A:

has_cycle_dfs(C, A, [], 0):
    visited = [C]
    Explorar C → ... (no hay aristas de C)
    return False

wait_create_cycle(C, A):
    has_cycle_dfs(A, C, [], 0):  # Invertir búsqueda
        visited = [A]
        Explorar A → B:
            has_cycle_dfs(B, C, [A], 1):
                visited = [A, B]
                Explorar B → C:
                    has_cycle_dfs(C, C, [A,B], 2):
                        from == to  → return True ✓
                    return True
                return True
            return True
        return True
    return True

RESULTADO: CICLO DETECTADO (A → B → C → A)
```

---

## 🎯 Decisiones de Diseño

### 1. Lock Classes = Direcciones de Memoria

**Decisión**: Usar la dirección virtual del lock como su "clase".

**Razón**:
- Simple y eficiente
- No requiere estructuras adicionales
- Único por lock

**Alternativa considerada**: Usar `lock_id` (índice en el array)
- Más estable si se reutiliza memoria
- Requiere modificar más código

**Elegido**: Dirección (`lock_addr`) por simplicidad.

### 2. Lista Enlazada vs Array

**Decisión**: Usar listas enlazadas para dependencias y held_locks.

**Razón**:
- Más eficiente en memoria (solo aloca lo necesario)
- Inserción O(1)
- Compatible con allocación dinámica de Selfie

**Alternativa considerada**: Arrays estáticos
- Más rápido acceso aleatorio
- Requiere pre-alocar memoria
- Difícil determinar tamaño óptimo

**Elegido**: Listas enlazadas por flexibilidad.

### 3. Detección Proactiva vs Reactiva

**Decisión**: Detección **proactiva** (antes de deadlock).

**Razón**:
- Previene el problema en lugar de detectarlo post-mortem
- Permite continuar ejecución después del warning
- Compatible con filosofía de Lockdep de Linux

**Alternativa considerada**: Detección reactiva (timeout)
- Solo detecta deadlocks reales
- No detecta potenciales
- Requiere timer y recovery complejos

**Elegido**: Proactiva por prevención.

### 4. Grafo Global vs Por-Proceso

**Decisión**: Grafo de dependencias **global** compartido.

**Razón**:
- Detecta deadlocks entre procesos
- Más eficiente en memoria
- Refleja realidad del sistema

**Alternativa considerada**: Grafo por proceso
- Más simple de implementar
- No detecta deadlocks inter-proceso
- Duplicación de información

**Elegido**: Global por completitud.

---

## 📊 Overhead y Rendimiento

### Memoria

| Componente | Tamaño | Cantidad Máxima | Total |
|------------|--------|-----------------|-------|
| Dependency | 24 B | 512 | ~12 KB |
| Held Lock | 16 B | 16 × N contexts | ~256 B × N |
| Context Extension | 16 B | N contexts | 16 B × N |
| Visited Buffer | 128 B | 1 (global) | 128 B |

**Total para 10 contextos**: ~15 KB

### Tiempo

| Operación | Complejidad | Típico | Peor Caso |
|-----------|-------------|--------|-----------|
| lock_acquire | O(H × (D + E)) | O(2 × 20) ≈ 40 ops | O(16 × 528) ≈ 8K ops |
| lock_release | O(H) | O(2) | O(16) |
| add_dependency | O(1) | 1 op | 1 op |
| cycle_detection | O(E + V) | O(20) | O(528) |

Donde:
- H = held_locks actualmente poseídos
- D = dependencias del grafo
- E = aristas exploradas en DFS
- V = vértices visitados en DFS

**Overhead estimado**: 1-5% en casos típicos, 10-20% en peor caso.

---

## 🔒 Restricciones de C* Respetadas

### 1. Sin `#define`

❌ **No usado**: `#define MAX_DEPS 512`

✅ **Usado**: `uint64_t MAX_LOCKDEP_DEPENDENCIES = 512;`

### 2. Sin operadores lógicos (`&&`, `||`)

❌ **No usado**:
```c
if (a == 0 && b == 1) { ... }
```

✅ **Usado**:
```c
if (a == 0) {
    if (b == 1) {
        ...
    }
}
```

### 3. Sin inicialización en declaración

❌ **No usado**:
```c
uint64_t count = 0;
```

✅ **Usado**:
```c
uint64_t count;
count = 0;
```

### 4. Solo punteros y uint64_t

✅ Todas las estructuras usan:
- `uint64_t` para valores
- `uint64_t *` para punteros

### 5. Sin structs nativos

❌ **No usado**:
```c
struct dependency {
    uint64_t from;
    uint64_t to;
};
```

✅ **Usado**:
```c
// Array de uint64_t con getters/setters
uint64_t get_dependency_from(uint64_t *dep) { return *dep; }
uint64_t get_dependency_to(uint64_t *dep) { return *(dep + 1); }
```

---

## 🧪 Casos de Prueba

### Test 1: Deadlock Simple AB-BA

```c
lock_acquire(A);  // Crea A
lock_acquire(B);  // Crea A→B
lock_release(B);
lock_release(A);

lock_acquire(B);  // Crea B
lock_acquire(A);  // Intenta B→A → DETECTA CICLO A→B→A ✓
```

**Resultado Esperado**: WARNING y rechazo de adquisición.

### Test 2: Sin Deadlock (Orden Consistente)

```c
lock_acquire(A);
lock_acquire(B);
lock_release(B);
lock_release(A);

lock_acquire(A);
lock_acquire(B);  // Misma dependencia A→B, OK ✓
lock_release(B);
lock_release(A);
```

**Resultado Esperado**: Sin warnings.

### Test 3: Cadena de 3 Locks

```c
lock_acquire(A);
lock_acquire(B);  // A→B
lock_acquire(C);  // B→C
lock_release(C);
lock_release(B);
lock_release(A);

lock_acquire(C);
lock_acquire(A);  // Intenta C→A → DETECTA CICLO A→B→C→A ✓
```

**Resultado Esperado**: WARNING mostrando cadena completa.

---

## 🚀 Extensiones Futuras

### 1. Hash Table para Dependencias

Reemplazar lista enlazada con tabla hash para búsqueda O(1).

```c
#define HASH_SIZE 1024
uint64_t *dependency_hash_table[HASH_SIZE];

uint64_t hash(uint64_t from, uint64_t to) {
    return (from + to) % HASH_SIZE;
}
```

### 2. Stack Traces

Guardar call stack cuando se crea cada dependencia.

```c
struct dependency {
    uint64_t from;
    uint64_t to;
    uint64_t *stack_trace;  // Puntero a array de PCs
    uint64_t stack_depth;
};
```

### 3. Estadísticas

Mantener contadores de locks más problemáticos.

```c
struct lock_stats {
    uint64_t lock_class;
    uint64_t acquisitions;
    uint64_t contentions;
    uint64_t deadlock_warnings;
};
```

### 4. Lock Ordering Rules

Permitir definir reglas de ordenamiento explícitas.

```c
void lockdep_add_ordering_rule(uint64_t before, uint64_t after);
```

---

## 📚 Referencias Técnicas

### Inspiración: Linux Lockdep

El sistema está inspirado en:
- **Archivo**: `kernel/locking/lockdep.c`
- **Autores**: Ingo Molnar, Peter Zijlstra
- **Conceptos clave**:
  - Lock classes
  - Dependency graph
  - Circular dependency detection
  - Lock state tracking

### Diferencias con Linux Lockdep

| Aspecto | Linux Lockdep | Selfie Lockdep |
|---------|---------------|----------------|
| Lock Classes | Estáticas con `lock_class_key` | Dinámicas (direcciones) |
| Hash Tables | Sí, múltiples | No, listas enlazadas |
| IRQ Context | Tracking completo | No aplicable |
| Stack Traces | Sí, con kallsyms | No (futuro) |
| Max Dependencies | ~32K | 512 |
| Performance | Altamente optimizado | Suficiente para Selfie |

---

## ✅ Validación del Diseño

### Criterios Cumplidos

- ✅ Detección proactiva de deadlocks
- ✅ Compatible con locks y semáforos
- ✅ Respeta restricciones de C*
- ✅ No rompe código existente
- ✅ Overhead aceptable
- ✅ Mensajes de error informativos
- ✅ Fácil de integrar
- ✅ Fácil de desactivar

### Tests Pasados

- ✅ Deadlock AB-BA detectado
- ✅ Orden consistente permitido
- ✅ Cadena de dependencias detectada
- ✅ Locks anidados funcionan
- ✅ Tests existentes de Selfie pasan

---

**Autor**: Sistema Lockdep para Selfie OS  
**Fecha**: 2025  
**Versión**: 1.0  
**Licencia**: Compatible con licencia de Selfie (BSD)
