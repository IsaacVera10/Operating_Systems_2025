# Implementación de Schedulers en Selfie

## Resumen
Este documento describe la implementación de **tres schedulers seleccionables en tiempo de ejecución** para el sistema Selfie: Round-Robin (predeterminado), Random Scheduler (baseline), y Completely Fair Scheduler (CFS).

## Ubicación en el Código

### Variables Globales (líneas ~2048-2061)
```c
// Scheduler type selection
uint64_t SCHEDULER_ROUND_ROBIN = 0;
uint64_t SCHEDULER_RANDOM = 1;
uint64_t SCHEDULER_CFS = 2;

uint64_t scheduler_type = 0; // default: round-robin (SCHEDULER_ROUND_ROBIN)
uint64_t random_seed = 12345; // seed for random number generator

uint64_t debug_scheduler = 0; // flag for debugging scheduler decisions
```

**Nota**: No se usan `#define` porque el compilador de selfie no los soporta. Se usan variables `uint64_t` constantes.

### Estructura del Contexto (PCB)

Se agregó un nuevo campo al PCB:
- **Posición 35**: `vruntime` - Virtual runtime para CFS scheduler

```c
uint64_t get_vruntime(uint64_t *context) { return *(context + 35); }
void set_vruntime(uint64_t *context, uint64_t vruntime) { *(context + 35) = vruntime; }
```

### Funciones del Scheduler (antes de handle_exception, ~línea 14050)

#### 1. **simple_rand()** - Generador de números pseudoaleatorios
```c
uint64_t simple_rand() {
  uint64_t temp;
  
  temp = random_seed * 1103515245;
  temp = temp + 12345;
  random_seed = temp % 2147483648; // 0x7FFFFFFF + 1
  
  return random_seed;
}
```
- Implementa un Linear Congruential Generator (LCG)
- Usado para selección aleatoria de procesos
- **Nota**: No usa operador `&` porque se evita usar bitwise operations complejas

#### 2. **count_runnable_contexts()** - Contador de procesos ejecutables
```c
uint64_t count_runnable_contexts() {
  uint64_t *context;
  uint64_t count;

  context = used_contexts;
  count = 0;

  while (context != (uint64_t *)0) {
    if (get_blocked(context) == 0)
      count = count + 1;
    
    context = get_next_context(context);
  }

  return count;
}
```
- Recorre la lista de contextos
- Cuenta solo los que están en estado "runnable" (get_blocked() == 0)

#### 3. **select_random_context()** - Selector aleatorio
```c
uint64_t *select_random_context() {
  uint64_t num_runnable;
  uint64_t random_index;
  uint64_t *context;
  uint64_t i;

  num_runnable = count_runnable_contexts();

  if (num_runnable == 0)
    return (uint64_t *)0;

  random_index = simple_rand();
  random_index = random_index % num_runnable;

  context = used_contexts;
  i = 0;

  while (context != (uint64_t *)0) {
    if (get_blocked(context) == 0) {
      if (i == random_index)
        return context;
      i = i + 1;
    }
    context = get_next_context(context);
  }

  return (uint64_t *)0;
}
```
- Selecciona un índice aleatorio entre [0, num_runnable-1]
- Retorna el contexto en esa posición

#### 4. **select_cfs_context()** - Selector CFS (Completely Fair Scheduler)
```c
uint64_t *select_cfs_context() {
  uint64_t *context;
  uint64_t *min_context;
  uint64_t min_vruntime;

  context = used_contexts;
  min_context = (uint64_t *)0;
  min_vruntime = UINT64_MAX;

  while (context != (uint64_t *)0) {
    if (get_blocked(context) == 0) {
      if (get_vruntime(context) < min_vruntime) {
        min_vruntime = get_vruntime(context);
        min_context = context;
      }
    }
    context = get_next_context(context);
  }

  return min_context;
}
```
- Recorre todos los contextos ejecutables
- Selecciona el que tiene el **menor vruntime**
- Garantiza equidad en el tiempo de CPU

### Modificación en mipster() (~línea 14165)

La función `mipster()` ahora incluye lógica para seleccionar entre tres schedulers:

```c
else {
  // Scheduler selection
  if (scheduler_type == SCHEDULER_RANDOM) {
    // Random Scheduler
    to_context = select_random_context();
    
    if (debug_scheduler) {
      if (to_context != (uint64_t *)0)
        printf("[SCHEDULER] Random: selecting process PID=%lu\n", get_id_context(to_context));
    }
    
    if (to_context == (uint64_t *)0) {
      // No runnable contexts, check if all are terminated
      // ... código de manejo ...
    }
  } else if (scheduler_type == SCHEDULER_CFS) {
    // Completely Fair Scheduler (CFS)
    to_context = select_cfs_context();
    
    if (debug_scheduler) {
      if (to_context != (uint64_t *)0)
        printf("[SCHEDULER] CFS: selecting process PID=%lu with vruntime=%lu\n", 
               get_id_context(to_context), get_vruntime(to_context));
    }
    
    if (to_context == (uint64_t *)0) {
      // No runnable contexts, check if all are terminated
      // ... código de manejo ...
    }
  } else {
    // Round-Robin Scheduler (default)
    to_context = get_next_context(from_context);
    if (to_context == (uint64_t *)0)
      to_context = used_contexts;

    start_ctx = to_context;

    while (get_blocked(to_context) >= 1) {
      // Buscar siguiente contexto ejecutable
      // ... código existente ...
    }
    
    if (debug_scheduler)
      printf("[SCHEDULER] Round-Robin: switching from PID=%lu to PID=%lu\n", 
             get_id_context(from_context), get_id_context(to_context));
  }

  // Update vruntime for CFS (increment by TIMESLICE)
  if (scheduler_type == SCHEDULER_CFS) {
    set_vruntime(from_context, get_vruntime(from_context) + TIMESLICE);
    
    if (debug_scheduler)
      printf("[SCHEDULER] CFS: updated PID=%lu vruntime to %lu\n", 
             get_id_context(from_context), get_vruntime(from_context));
  }

  timeout = TIMESLICE;
}
```

**Nota importante**: El log de debug del scheduler Round-Robin se mueve **después** del bucle `while` que busca contextos ejecutables, para que muestre el contexto final que realmente se va a ejecutar.

## Uso

### Opciones de Línea de Comandos

```bash
./selfie [-scheduler <tipo>] [-debug-scheduler] [otras opciones]
```

Donde `<tipo>` puede ser:
- **rr** - Round-Robin (predeterminado)
- **random** - Random Scheduler
- **cfs** - Completely Fair Scheduler

Y `-debug-scheduler` activa logs detallados del scheduler.

### Ejemplos

**Round-Robin (default):**
```bash
./selfie -c programa.c -o programa.m -m 1
# o explícitamente
./selfie -scheduler rr -c programa.c -o programa.m -m 1
```

**Random Scheduler:**
```bash
./selfie -scheduler random -c programa.c -o programa.m -m 1
```

**CFS:**
```bash
./selfie -scheduler cfs -c programa.c -o programa.m -m 1
```

**Con debug activado:**
```bash
./selfie -scheduler cfs -l programa.m -debug-scheduler -m 1
```

## Schedulers Implementados

### 1. Round-Robin (RR) - Default

**Características:**
- Scheduler predeterminado de selfie
- Selección circular: recorre los contextos en orden secuencial
- Fair scheduling: cada proceso recibe tiempo de CPU equitativo
- Predecible y determinista

**Comportamiento:**
- Recorre la lista `used_contexts` en orden
- Salta procesos bloqueados o terminados
- Vuelve al inicio de la lista cuando llega al final

**Ventajas:**
- Simple y eficiente
- Predecible
- Sin overhead adicional

**Desventajas:**
- No considera prioridades
- No optimiza para procesos interactivos

---

### 2. Random Scheduler - Baseline

**Características:**
- Selección aleatoria de procesos
- Sin prioridades ni historial
- Útil como baseline para comparaciones

**Comportamiento:**
- Cuenta procesos ejecutables
- Genera un índice aleatorio
- Selecciona el proceso en esa posición

**Ventajas:**
- Implementación simple
- No requiere estado adicional
- Bueno para testing y comparaciones
- Fairness a largo plazo

**Desventajas:**
- Impredecible
- Puede seleccionar el mismo proceso varias veces consecutivas
- Mayor latencia promedio
- No óptimo para producción

---

### 3. Completely Fair Scheduler (CFS)

**Características:**
- Basado en tiempo de ejecución virtual (vruntime)
- Garantiza equidad perfecta
- Inspirado en el CFS de Linux

**Comportamiento:**
- Mantiene `vruntime` para cada proceso
- Selecciona siempre el proceso con **menor vruntime**
- Incrementa vruntime en TIMESLICE (100000) después de cada ejecución
- Los procesos convergen a vruntime similares

**Ventajas:**
- Equidad garantizada matemáticamente
- No hay inanición (starvation)
- Balanceo automático de carga
- Óptimo para sistemas multiproceso

**Desventajas:**
- Overhead de búsqueda O(n) en cada selección
- Requiere campo adicional en PCB (vruntime)
- Más complejo que Round-Robin

**Implementación:**
- `vruntime` se inicializa a 0 en nuevos contextos
- Se copia del padre al hijo en `fork()`
- Se incrementa en TIMESLICE después de cada quantum
- Se mantiene mientras el proceso exista

---

## Debug y Trazas

### Activación de Debug

```bash
./selfie -scheduler <tipo> -l programa.m -debug-scheduler -m 1
```

### Logs Generados

**Round-Robin:**
```
[SCHEDULER] Round-Robin: switching from PID=1 to PID=2
```

**Random:**
```
[SCHEDULER] Random: selecting process PID=3
```

**CFS:**
```
[SCHEDULER] CFS: selecting process PID=2 with vruntime=200000
[SCHEDULER] CFS: updated PID=2 vruntime to 300000
```

**Fork:**
```
[FORK] Parent PID=0 created child PID=1
```

Los logs permiten:
- Verificar el comportamiento del scheduler
- Identificar problemas de equidad
- Comparar diferentes schedulers
- Debugging de deadlocks

## Próximos Pasos

Para implementar el **Completely Fair Scheduler (CFS)**:

1. Agregar campo `vruntime` (virtual runtime) a cada contexto
2. Implementar estructura de datos para mantener procesos ordenados por vruntime
3. Seleccionar siempre el proceso con menor vruntime
4. Actualizar vruntime después de cada ejecución

## Pruebas y Resultados

### Programa de Prueba 1: `test_scheduler.c`
```c
uint64_t main() {
  uint64_t pid1, pid2, pid3, i;

  pid1 = fork();
  if (pid1 == 0) {
    i = 0;
    while (i < 5) i = i + 1;
    exit(0);
  }
  
  pid2 = fork();
  if (pid2 == 0) {
    i = 0;
    while (i < 5) i = i + 1;
    exit(0);
  }
  
  pid3 = fork();
  if (pid3 == 0) {
    i = 0;
    while (i < 5) i = i + 1;
    exit(0);
  }
  
  i = 0;
  while (i < 10) i = i + 1;
  
  return 0;
}
```

### Programa de Prueba 2: `test_scheduler_print.c`
```c
uint64_t* string;

uint64_t main() {
  uint64_t pid1, pid2, pid3, i;

  string = "Parent process starting\n";
  write(1, string, 24);

  pid1 = fork();
  if (pid1 == 0) {
    string = "Process 1 executing\n";
    write(1, string, 20);
    i = 0;
    while (i < 3) i = i + 1;
    string = "Process 1 done\n";
    write(1, string, 15);
    exit(0);
  }
  
  // Similar para pid2 y pid3...
  
  string = "Parent waiting\n";
  write(1, string, 15);
  i = 0;
  while (i < 5) i = i + 1;
  string = "Parent done\n";
  write(1, string, 12);
  
  return 0;
}
```

### Programa de Prueba 3: `print.c` (2³ = 8 procesos)
```c
uint64_t* foo;

int main(int argc, char** argv) {
  // create 2^3 processes
  fork();
  fork();
  fork();

  foo = "Hello World!    ";
  
  while (*foo != 0) {
    write(1, foo, 8);
    foo = foo + 1;
  }
}
```

### Compilación y Ejecución
```bash
# Compilar
./selfie -c test_scheduler.c -o test_scheduler.m

# Ejecutar con Round-Robin
./selfie -l test_scheduler.m -m 1
./selfie -scheduler rr -l test_scheduler.m -m 1

# Ejecutar con Random Scheduler
./selfie -scheduler random -l test_scheduler.m -m 1

# Ejecutar con CFS
./selfie -scheduler cfs -l test_scheduler.m -m 1

# Con debug
./selfie -scheduler cfs -l test_scheduler.m -debug-scheduler -m 1
```

### Resultados Esperados

Todos los schedulers deben:
- ✅ Ejecutar todos los procesos hasta completarlos
- ✅ Retornar exit code 0
- ✅ Mostrar estadísticas similares de instrucciones ejecutadas
- ✅ No causar deadlocks ni starvation

**Diferencias observadas:**
- **Round-Robin**: Orden predecible y secuencial
- **Random**: Orden impredecible, puede repetir procesos
- **CFS**: Orden basado en vruntime, convergencia a equidad

## Verificación

Ambos schedulers deben:
- Ejecutar todos los procesos hasta completarlos
- Retornar exit code 0
- Mostrar estadísticas similares de instrucciones ejecutadas

## Referencias

- Código ubicado en `selfie.c`
- Scheduler se ejecuta en la función `mipster()` (línea ~14069)
- Funciones auxiliares antes de `handle_exception()` (línea ~14043)
