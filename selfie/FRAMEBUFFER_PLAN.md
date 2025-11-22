# Plan de Trabajo: Driver de Pantalla Emulada (Framebuffer MMIO)

## Objetivo
Desarrollar un sistema de E/S gráfica minimalista que demuestre la interacción entre el Guest OS (Selfie) y el Host Emulator mediante Memory-Mapped I/O (MMIO).

---

## Fase 1: Análisis y Diseño Preliminar (Estimado: 2-3 horas)

### 1.1 Comprensión del Sistema Actual
- [ ] Revisar la arquitectura de emulación de Selfie
- [ ] Identificar cómo funciona actualmente `store_word()` en el emulador
- [ ] Localizar el manejo de registros de memoria en el código
- [ ] Documentar las convenciones de memoria virtual vs física

### 1.2 Definición de Constantes MMIO
- [ ] Definir `FB_START`: dirección base del framebuffer en memoria
- [ ] Definir `DRAW_REGISTER`: dirección del registro de control
- [ ] Definir dimensiones del framebuffer (ancho, alto)
- [ ] Establecer formato de color (RGB, escala de grises, etc.)

### 1.3 Decisiones de Diseño Clave
- [ ] **Caché**: Determinar si la región MMIO debe ser cacheable o no-cacheable
  - Investigar si Selfie simula caché de datos
  - Definir política de coherencia si es necesario
- [ ] **Formato de Pixel**: Decidir representación (32-bit RGBA, 24-bit RGB, etc.)
- [ ] **Tamaño del Buffer**: Calcular memoria necesaria (ej. 640x480x4 bytes)

---

## Fase 2: Extensión del Emulador (Host) (Estimado: 4-6 horas)

### 2.1 Configuración de la Ventana SDL
- [ ] Agregar dependencia de SDL2 al proyecto
- [ ] Modificar `Makefile` para enlazar con `-lSDL2`
- [ ] Crear función `init_framebuffer()`:
  - Inicializar SDL
  - Crear ventana
  - Crear renderer
  - Crear textura para el framebuffer
- [ ] Agregar cleanup en `exit_selfie()` o equivalente

### 2.2 Modificación de `store_word()`
- [ ] Localizar la función `store_word()` en `selfie.c`
- [ ] Implementar detección de escritura en `DRAW_REGISTER`:
  ```c
  if (vaddr == DRAW_REGISTER) {
      trigger_framebuffer_refresh();
      return;
  }
  ```
- [ ] Implementar detección de escritura en rango `FB_START`:
  ```c
  if (vaddr >= FB_START && vaddr < FB_START + FB_SIZE) {
      uint64_t offset = vaddr - FB_START;
      framebuffer[offset / 4] = value; // Almacenar pixel
      return;
  }
  ```

### 2.3 Renderizado de la Pantalla
- [ ] Implementar `refresh_screen()`:
  - Copiar datos del framebuffer a la textura SDL
  - Actualizar renderer
  - Presentar en pantalla
- [ ] Manejar eventos SDL (cerrar ventana, etc.)
- [ ] Implementar loop de eventos no-bloqueante

---

## Fase 3: Implementación del Driver (Guest) (Estimado: 3-4 horas)

### 3.1 Archivo de Driver (`framebuffer_driver.c`)
- [ ] Crear archivo nuevo en `/examples/` o raíz del proyecto
- [ ] Definir constantes:
  ```c
  #define FB_START 0x80000000
  #define DRAW_REGISTER 0x80001000
  #define FB_WIDTH 640
  #define FB_HEIGHT 480
  ```

### 3.2 Implementar Primitivas Básicas
- [ ] `draw_pixel(uint64_t x, uint64_t y, uint32_t color)`:
  - Calcular offset: `offset = (y * FB_WIDTH + x) * 4`
  - Escribir en `FB_START + offset`
- [ ] `refresh_screen()`:
  - Escribir cualquier valor en `DRAW_REGISTER`
- [ ] `clear_screen(uint32_t color)`:
  - Llenar todo el framebuffer con un color

### 3.3 Programa de Demostración
- [ ] Crear `test_framebuffer.c`:
  - Dibujar un patrón simple (gradiente, líneas, rectángulo)
  - Llamar a `refresh_screen()`
  - Demostrar animación simple (opcional)

---

## Fase 4: Integración y Testing (Estimado: 3-4 horas)

### 4.1 Compilación
- [ ] Compilar el emulador extendido:
  ```bash
  make clean
  make selfie
  ```
- [ ] Compilar el programa de prueba con Selfie:
  ```bash
  ./selfie -c test_framebuffer.c -o test_framebuffer.m
  ```

### 4.2 Pruebas Básicas
- [ ] Test 1: Inicialización de ventana
  - Verificar que SDL se inicializa correctamente
  - Ventana aparece sin errores
- [ ] Test 2: Escritura de un pixel
  - Dibujar un solo pixel y verificar visibilidad
- [ ] Test 3: Llenar pantalla
  - Probar `clear_screen()` con diferentes colores
- [ ] Test 4: Patrón complejo
  - Dibujar gradiente o patrón geométrico

### 4.3 Debugging
- [ ] Agregar prints de debug en `store_word()` para verificar detección MMIO
- [ ] Validar rangos de direcciones
- [ ] Verificar sincronización entre escrituras y refresh

---

## Fase 5: Optimización y Documentación (Estimado: 2-3 horas)

### 5.1 Optimizaciones (Opcional)
- [ ] Implementar double-buffering
- [ ] Reducir frecuencia de refresh (solo cuando sea necesario)
- [ ] Optimizar copia de memoria del framebuffer

### 5.2 Documentación
- [ ] Documentar constantes y convenciones de memoria
- [ ] Agregar comentarios en el código
- [ ] Crear ejemplos adicionales (dibujar texto ASCII, formas básicas)
- [ ] Actualizar README con instrucciones de compilación

---

## Fase 6: Análisis de Preguntas Clave (Estimado: 2-3 horas)

### 6.1 Ventajas de MMIO
- [ ] Escribir análisis sobre por qué MMIO es preferible a port-mapped I/O:
  - Unificación del espacio de direcciones
  - No requiere instrucciones especiales (in/out)
  - Simplifica el diseño del hardware
  - Mejor para arquitecturas RISC

### 6.2 Rol de la Caché
- [ ] Investigar simulación de caché en Selfie (si existe)
- [ ] Documentar problemas de coherencia:
  - Write-through vs write-back
  - Stale data en lecturas
  - Necesidad de uncacheable MMIO regions
- [ ] Proponer solución: marcar región MMIO como no-cacheable

### 6.3 Diseño de Abstracción
- [ ] Justificar separación Host/Guest:
  - SDL requiere bibliotecas del sistema operativo real
  - C estándar en Guest vs APIs nativas en Host
  - Portabilidad y modularidad
  - Seguridad: aislar código del emulador

---

## Cronograma Sugerido

| Fase | Duración | Dependencias |
|------|----------|--------------|
| Fase 1 | 2-3 h | Ninguna |
| Fase 2 | 4-6 h | Fase 1 |
| Fase 3 | 3-4 h | Fase 1 |
| Fase 4 | 3-4 h | Fases 2 y 3 |
| Fase 5 | 2-3 h | Fase 4 |
| Fase 6 | 2-3 h | Fase 4 |

**Total estimado: 16-23 horas**

---

## Entregables

1. ✅ Código del emulador modificado (`selfie.c`)
2. ✅ Driver del framebuffer (`framebuffer_driver.c` o integrado en programa de prueba)
3. ✅ Programa de demostración (`test_framebuffer.c`)
4. ✅ Makefile actualizado
5. ✅ Documentación de diseño
6. ✅ Respuestas a preguntas de análisis

---

## Posibles Obstáculos

### Problema: SDL no disponible
**Solución**: Instalar con `sudo apt-get install libsdl2-dev` (Ubuntu/Debian) o equivalente

### Problema: Direcciones MMIO conflictúan con memoria existente
**Solución**: Usar direcciones altas (ej. 0x80000000+) fuera del rango de memoria del programa

### Problema: Rendimiento lento
**Solución**: 
- Reducir resolución del framebuffer
- Implementar dirty rectangles (solo actualizar regiones modificadas)
- Limitar FPS de refresh

### Problema: Selfie no tiene soporte de punteros a memoria MMIO
**Solución**: Usar ensamblador inline o syscalls especiales para escrituras MMIO

---

## Recursos Útiles

- Documentación SDL2: https://wiki.libsdl.org/SDL2/FrontPage
- MMIO en sistemas embebidos: buscar ejemplos de Raspberry Pi bare-metal
- Código fuente de Selfie: revisar manejo de memoria virtual
- RISC-V MMIO: especificación de mapeo de dispositivos

---

## Notas Adicionales

- **Modularidad**: Mantener el código del framebuffer separado para facilitar testing
- **Versionado**: Hacer commits incrementales (init, store_word, SDL, driver, tests)
- **Testing Incremental**: No esperar a terminar todo para probar; validar cada componente
- **Flexibilidad**: Este plan puede ajustarse según hallazgos durante la implementación

---

**Última actualización**: 22 de noviembre de 2025
