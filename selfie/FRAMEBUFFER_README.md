# Framebuffer MMIO Driver - Quick Start

## Compilar y Ejecutar

```bash
# 1. Compilar selfie con SDL2
make clean && make selfie

# 2. Compilar programa de prueba
./selfie -c examples/test_framebuffer.c -o examples/test_framebuffer.m

# 3. Ejecutar
./selfie -l examples/test_framebuffer.m -m 1
```

## Arquitectura

```
Guest (C*) → MMIO Write → Host (selfie + SDL) → Display

FB_START (0x80000000):    Framebuffer 320x240 RGBA
DRAW_REGISTER (0x90000000): Trigger screen refresh
```

## API del Driver (Guest)

```c
// Dibujar un píxel
void draw_pixel(uint64_t x, uint64_t y, uint64_t color);

// Limpiar pantalla
void clear_screen(uint64_t color);

// Dibujar rectángulo
void draw_rectangle(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t color);

// Actualizar pantalla (obligatorio después de dibujar)
void refresh_screen();
```

## Formato de Color

RGBA little-endian (32-bit): `0xAABBGGRR`

```c
uint64_t RED   = 4278190335;  // 0xFF0000FF
uint64_t GREEN = 4278255360;  // 0xFF00FF00
uint64_t BLUE  = 4294901760;  // 0xFFFF0000
uint64_t BLACK = 4278190080;  // 0xFF000000
```

## Cómo Funciona

1. **Guest escribe a FB_START:** Host detecta en `do_store()` y actualiza `framebuffer_data[]`
2. **Guest escribe a DRAW_REGISTER:** Host llama `refresh_framebuffer()` → SDL renderiza
3. **SDL muestra:** Ventana 640x480 (2x scale) con contenido del framebuffer

## Documentación Completa

Ver `FRAMEBUFFER_IMPLEMENTATION.md` para:
- Análisis de ventajas de MMIO
- Rol de la caché y coherencia
- Justificación de diseño Host/Guest
