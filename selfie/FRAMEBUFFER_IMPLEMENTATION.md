# Driver de Pantalla Emulada (Framebuffer MMIO) - Documentación Completa

## Resumen Ejecutivo

Se ha implementado con éxito un sistema de E/S gráfica minimalista que demuestra la interacción entre el Guest OS (Selfie) y el Host Emulator mediante Memory-Mapped I/O (MMIO). El sistema está completamente funcional y muestra rectángulos de colores en una ventana SDL.

---

## Arquitectura del Sistema

### Componentes Implementados

1. **Host Emulator (selfie.c)**
   - Extensión con SDL2 para crear ventana de visualización
   - Modificación de `do_store()` para interceptar escrituras MMIO
   - Framebuffer de 320x240 píxeles (RGBA)

2. **Guest Driver (test_framebuffer.c)**
   - Funciones primitivas: `draw_pixel()`, `clear_screen()`, `draw_rectangle()`
   - Función `refresh_screen()` para activar el renderizado

3. **Interfaz MMIO**
   - `FB_START = 0x80000000`: Inicio del framebuffer en memoria virtual
   - `DRAW_REGISTER = 0x90000000`: Registro de control para refresh
   - Tamaño: 320×240×4 bytes = 307,200 bytes

---

## Implementación Detallada

### 1. Extensión del Emulador (Host)

#### Inicialización SDL (init_framebuffer())
```c
- SDL_Init(SDL_INIT_VIDEO)
- Creación de ventana 640x480 (2x escala para mejor visibilidad)
- Renderer acelerado por hardware
- Textura RGBA8888 de 320x240
- Framebuffer host-side de 307KB
```

#### Interceptación MMIO en do_store()
```c
uint64_t do_store() {
    vaddr = *(registers + rs1) + imm;
    
    // Caso 1: Escritura en DRAW_REGISTER → refresh
    if (vaddr == DRAW_REGISTER) {
        refresh_framebuffer();
        return;
    }
    
    // Caso 2: Escritura en FB_START..FB_START+FB_SIZE → píxel
    if (vaddr >= FB_START && vaddr < FB_START + FB_SIZE) {
        uint64_t offset = vaddr - FB_START;
        uint64_t pixel_index = offset / 4;
        framebuffer_data[pixel_index] = *(registers + rs2);
        return;
    }
    
    // Caso 3: Memoria normal → continuar con flujo estándar
    ...
}
```

#### Renderizado (refresh_framebuffer())
```c
- Procesa eventos SDL (cierre de ventana)
- SDL_UpdateTexture() con framebuffer_data
- SDL_RenderCopy() y SDL_RenderPresent()
```

### 2. Driver del Guest (C*)

#### Escritura de Píxeles
```c
void draw_pixel(uint64_t x, uint64_t y, uint64_t color) {
    offset = y * FB_WIDTH + x;
    offset = offset * 4;
    pixel_addr = (uint64_t*) (FB_START + offset);
    *pixel_addr = color;  // Escritura interceptada por Host
}
```

#### Trigger de Refresh
```c
void refresh_screen() {
    uint64_t* reg = (uint64_t*) DRAW_REGISTER;
    *reg = 1;  // Escritura detectada → refresh
}
```

### 3. Programa de Demostración

El programa muestra:
1. Pantalla negra (inicialización)
2. Rectángulo rojo (50,50, 100x80)
3. Rectángulo verde (100,80, 120x60)
4. Rectángulo azul (150,110, 80x100)
5. Múltiples rectángulos de colores

---

## Análisis de Preguntas Clave

### I) Ventajas de MMIO sobre Port-Mapped I/O

**Memory-Mapped I/O es superior en este contexto porque:**

1. **Unificación del Espacio de Direcciones**
   - Los dispositivos se acceden con las mismas instrucciones LOAD/STORE
   - No requiere instrucciones especiales (IN/OUT de x86)
   - Simplifica el conjunto de instrucciones del procesador

2. **Compatibilidad con RISC-U**
   - RISC-U solo tiene `ld` (load) y `sd` (store)
   - No tiene instrucciones de I/O dedicadas
   - MMIO es la única opción arquitecturalmente viable

3. **Transparencia para el Compilador**
   - El compilador trata el framebuffer como memoria normal
   - No necesita generar código especial
   - Los punteros funcionan naturalmente

4. **Seguridad y Virtualización**
   - El MMU puede controlar acceso a regiones MMIO
   - Page faults protegen dispositivos críticos
   - Facilita la virtualización de hardware

5. **Rendimiento en Arquitecturas Modernas**
   - Cacheable/non-cacheable se configura por página
   - DMA puede acceder directamente (same address space)
   - Mejor para sistemas de 64 bits con espacio amplio

**Desventaja principal:** Consume espacio de direcciones virtuales (pero con 64 bits esto no es problema)

---

### II) Rol de la Caché y Problemas de Coherencia

#### Estado Actual en Selfie
Selfie simula caché L1 (ver `L1_CACHE_ENABLED` en el código). Las funciones `store_cached_virtual_memory()` y `load_cached_virtual_memory()` manejan la caché de datos.

#### Problema: MMIO Regions Deben ser Uncacheable

**¿Por qué?**

1. **Write-through vs Write-back**
   ```
   Escenario con caché write-back:
   1. CPU escribe color rojo en FB_START+0x1000
   2. Dato queda en caché, NO llega al framebuffer real
   3. refresh_screen() lee datos viejos → píxel no aparece
   ```

2. **Stale Data en Lecturas**
   ```
   Escenario con dispositivos que escriben al framebuffer:
   1. GPU actualiza píxel en memoria
   2. CPU lee mismo píxel desde caché
   3. CPU lee valor viejo (stale data)
   ```

3. **Memory-Mapped Registers**
   ```
   DRAW_REGISTER puede ser read-write:
   - Lectura retorna estado del dispositivo
   - Si está en caché, leemos estado viejo
   - Firmware crash por polling incorrecto
   ```

#### Solución Implementada

En nuestra implementación, **la caché NO es un problema** porque:

1. **Detección Temprana en do_store()**
   ```c
   // Interceptamos ANTES de llegar a store_cached_virtual_memory()
   if (vaddr == DRAW_REGISTER || 
       (vaddr >= FB_START && vaddr < FB_START + FB_SIZE)) {
       // Escribimos DIRECTAMENTE al framebuffer_data del host
       framebuffer_data[pixel_index] = value;
       return;  // NO pasamos por caché
   }
   ```

2. **Bypass Completo**
   - Las escrituras MMIO **NO** entran al sistema de memoria virtual
   - **NO** pasan por `store_cached_virtual_memory()`
   - Van directamente a `framebuffer_data[]` del host

#### Solución en Hardware Real

En un sistema operativo real:

```c
// Marcar página como uncacheable en page table
PTE pte;
pte.physical_address = 0x80000000;  // FB_START
pte.cacheable = 0;                  // Deshabilitar caché
pte.valid = 1;
```

Flags comunes:
- x86: `PAT` (Page Attribute Table) - bit UC (Uncacheable)
- ARM: `TEX/C/B` bits en PTE - Device memory type
- RISC-V: `PMA` (Physical Memory Attributes) - I/O region

---

### III) Diseño de Abstracción: ¿Por qué SDL en Host, no en Guest?

**Razones Arquitecturales:**

1. **Dependencias del Sistema Operativo Real**
   ```
   SDL2 requiere:
   - libX11.so, libwayland-client.so (Linux)
   - kernel32.dll, gdi32.dll (Windows)
   - Cocoa frameworks (macOS)
   
   El Guest ejecuta en un mundo virtual sin acceso a estos recursos.
   ```

2. **Syscalls del OS Real**
   ```c
   SDL_CreateWindow() internamente hace:
   - open("/dev/dri/card0")     → syscall open()
   - ioctl(fd, DRM_IOCTL_...)   → syscall ioctl()
   - mmap(..., PROT_READ|WRITE) → syscall mmap()
   
   Estas syscalls NO existen en el emulador simplificado de Selfie.
   ```

3. **Modelo de Ejecución**
   ```
   Host (selfie):      ejecuta NATIVAMENTE en Linux
   Guest (test.m):     ejecuta en EMULACIÓN (RISC-U virtual)
   
   SDL necesita: instrucciones x86-64, librerías compartidas, GPU drivers
   Guest solo tiene: instrucciones RISC-U emuladas, sin acceso a hardware
   ```

4. **Complejidad de C* vs C Completo**
   ```c
   C* NO soporta:
   - Structs (SDL_Window*, SDL_Event)
   - Function pointers (callbacks)
   - Enums (SDL_INIT_VIDEO)
   - Unions (SDL_Event)
   - Varargs (excepto printf)
   
   SDL2 API usa todo lo anterior intensivamente.
   ```

5. **Seguridad y Aislamiento**
   ```
   Principio de diseño de sistemas operativos:
   - Drivers en kernel space (privilegiado)
   - Aplicaciones en user space (no privilegiado)
   
   En nuestro modelo:
   - Host = kernel space (tiene acceso a hardware real)
   - Guest = user space (aislado, solo ve memoria virtual)
   ```

**Analogía con Hardware Real:**

```
┌────────────────────────────────┐
│   Aplicación (Guest)           │
│   draw_pixel(10, 20, RED);     │  ← Escribe a MMIO
└────────────────────────────────┘
            ↓ MMIO write
┌────────────────────────────────┐
│   Driver de Video (Host)       │
│   Detecta escritura en FB_START│
│   Actualiza framebuffer real   │  ← SDL
└────────────────────────────────┘
            ↓
┌────────────────────────────────┐
│   Hardware GPU                 │
│   Muestra píxeles en pantalla  │  ← Display físico
└────────────────────────────────┘
```

**Ventajas de esta Separación:**

1. **Portabilidad**: Guest code es portable, Host maneja detalles del OS
2. **Modularidad**: Se puede cambiar backend (SDL → OpenGL → framebuffer real)
3. **Debugging**: Errores en Guest no crashean la ventana
4. **Realismo**: Refleja arquitectura real de OS (kernel drivers vs userspace apps)

---

## Instrucciones de Uso

### Compilación
```bash
# 1. Instalar SDL2 (si no está instalado)
sudo apt-get install libsdl2-dev

# 2. Compilar selfie con soporte SDL
make clean
make selfie

# 3. Compilar programa de prueba
./selfie -c examples/test_framebuffer.c -o examples/test_framebuffer.m
```

### Ejecución
```bash
# Ejecutar con 1MB de memoria
./selfie -l examples/test_framebuffer.m -m 1
```

**Resultado esperado:**
- Se abre una ventana de 640x480
- Muestra secuencia de rectángulos de colores
- Programa termina automáticamente después de las animaciones

---

## Métricas de Implementación

### Código del Emulador
- **Líneas agregadas a selfie.c:** ~200 líneas
- **Funciones nuevas:** 4 (init, refresh, cleanup, handle_events)
- **Modificaciones:** 1 función (`do_store()`)

### Código del Guest
- **Líneas en test_framebuffer.c:** ~140 líneas
- **Funciones:** 5 (draw_pixel, clear_screen, draw_rectangle, delay, main)

### Rendimiento
- **Resolución:** 320×240 = 76,800 píxeles
- **Framebuffer size:** 307,200 bytes
- **Tiempo de clear_screen():** ~76,800 stores MMIO
- **Overhead:** Mínimo (detección en do_store es O(1))

---

## Pruebas Realizadas

### Test 1: Inicialización
✅ Ventana SDL se crea correctamente
✅ Framebuffer se inicializa a negro

### Test 2: Escritura de Píxeles
✅ `draw_pixel()` escribe colores correctos
✅ Coordenadas (x,y) funcionan correctamente

### Test 3: Refresh Trigger
✅ Escritura a `DRAW_REGISTER` actualiza pantalla
✅ Eventos SDL se procesan (cierre de ventana)

### Test 4: Múltiples Frames
✅ `clear_screen()` limpia entre frames
✅ Rectángulos se dibujan en posiciones correctas

---

## Problemas Encontrados y Soluciones

### Problema 1: Conflictos con Headers del Sistema
**Error:** `two or more data types in declaration specifiers`

**Causa:** Flag `-D'uint64_t=unsigned long'` en Makefile conflictúa con `<stdint.h>` de SDL

**Solución:**
```makefile
# Usar flags diferentes para build con SDL
CFLAGS_SDL := -Wall -Wextra -g $(shell pkg-config --cflags sdl2)
```
```c
// Incluir stdint.h antes de SDL
#include <stdint.h>
#include <SDL2/SDL.h>
```

### Problema 2: Conflicto con atoi()
**Error:** `conflicting types for 'atoi'`

**Causa:** SDL incluye `<stdlib.h>` que define `int atoi()`

**Solución:** Renombrar `atoi()` de selfie a `selfie_atoi()`

### Problema 3: Sintaxis C* Limitada
**Error:** `syntax error: ";" expected but "=" found`

**Causa:** C* no soporta expresiones complejas: `offset = (y * w + x) * 4`

**Solución:**
```c
// Mal (C estándar)
offset = (y * FB_WIDTH + x) * 4;

// Bien (C*)
offset = y * FB_WIDTH;
offset = offset + x;
offset = offset * 4;
```

---

## Extensiones Futuras

### Corto Plazo
1. **Soporte de Teclado/Mouse**
   - Interceptar eventos SDL
   - Mapear a registros MMIO de entrada
   - Implementar polling en Guest

2. **Double Buffering**
   - Dos framebuffers (front/back)
   - Swap atómico con registro de control
   - Elimina tearing visual

### Mediano Plazo
3. **Text Mode**
   - Framebuffer de caracteres ASCII
   - ROM con font bitmap 8x8
   - Terminal emulada completa

4. **Sprites y Blitting**
   - MMIO registers para blit operations
   - Hardware acceleration simulation
   - Game engine primitives

### Largo Plazo
5. **GPU Emulation**
   - Command buffers
   - Vertex/fragment shaders simulados
   - 3D pipeline básico

---

## Conclusiones

### Objetivos Cumplidos ✅
1. ✅ Sistema de E/S gráfica funcional
2. ✅ Demostración de interacción Guest-Host mediante MMIO
3. ✅ Arquitectura limpia y modular
4. ✅ Documentación completa del diseño

### Aprendizajes Clave
1. **MMIO como Abstracción Poderosa:** Simplifica la interfaz hardware-software
2. **Importancia de Uncacheable Regions:** Caché y MMIO requieren manejo especial
3. **Separación de Concerns:** Driver en host permite flexibilidad y portabilidad
4. **C* como Herramienta Educativa:** Limitaciones fuerzan entendimiento profundo

### Aplicabilidad Real
Este diseño es análogo a:
- **Raspberry Pi bare-metal framebuffer** (MMIO real)
- **QEMU device emulation** (interceptación similar)
- **Embedded graphics libraries** (sin OS, acceso directo)

---

**Implementación completa por:** Isaac Vera  
**Fecha:** 22 de noviembre de 2025  
**Sistema:** Selfie Educational OS  
**Líneas de código:** ~340 (Host + Guest)  
**Estado:** ✅ Totalmente funcional
