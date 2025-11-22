// Test program for Framebuffer MMIO driver
// Demonstrates interaction between Guest OS (C*) and Host Emulator (SDL)

// MMIO Constants (must match selfie.c)
uint64_t FB_START;
uint64_t DRAW_REGISTER;
uint64_t FB_WIDTH;
uint64_t FB_HEIGHT;

// Helper function to write a pixel to framebuffer
void draw_pixel(uint64_t x, uint64_t y, uint64_t color) {
    uint64_t offset;
    uint64_t* pixel_addr;
    
    if (x >= FB_WIDTH)
        return;
    if (y >= FB_HEIGHT)
        return;
    
    // Calculate pixel offset: (y * width + x) * 4 bytes
    offset = y * FB_WIDTH;
    offset = offset + x;
    offset = offset * 4;
    
    // Calculate absolute address
    pixel_addr = (uint64_t*) (FB_START + offset);
    
    // Write color to framebuffer
    *pixel_addr = color;
}

// Trigger screen refresh by writing to DRAW_REGISTER
void refresh_screen() {
    uint64_t* reg;
    reg = (uint64_t*) DRAW_REGISTER;
    *reg = 1;  // Any value triggers refresh
}

// Clear screen with a color
void clear_screen(uint64_t color) {
    uint64_t x;
    uint64_t y;
    
    y = 0;
    while (y < FB_HEIGHT) {
        x = 0;
        while (x < FB_WIDTH) {
            draw_pixel(x, y, color);
            x = x + 1;
        }
        y = y + 1;
    }
}

// Draw a filled rectangle
void draw_rectangle(uint64_t x0, uint64_t y0, uint64_t width, uint64_t height, uint64_t color) {
    uint64_t x;
    uint64_t y;
    uint64_t x_end;
    uint64_t y_end;
    
    x_end = x0 + width;
    y_end = y0 + height;
    
    y = y0;
    while (y < y_end) {
        x = x0;
        while (x < x_end) {
            draw_pixel(x, y, color);
            x = x + 1;
        }
        y = y + 1;
    }
}

// Simple delay function
void delay(uint64_t count) {
    uint64_t i;
    i = 0;
    while (i < count) {
        i = i + 1;
    }
}

uint64_t main() {
    uint64_t RED;
    uint64_t GREEN;
    uint64_t BLUE;
    uint64_t BLACK;
    
    // Initialize constants
    FB_START = 2147483648;      // 0x80000000
    DRAW_REGISTER = 2415919104; // 0x90000000
    FB_WIDTH = 320;
    FB_HEIGHT = 240;
    
    // Colors in RGBA format (0xAABBGGRR)
    RED = 4278190335;      // 0xFF0000FF
    GREEN = 4278255360;    // 0xFF00FF00
    BLUE = 4294901760;     // 0xFFFF0000
    BLACK = 4278190080;    // 0xFF000000
    
    // Frame 1: Clear screen to black
    clear_screen(BLACK);
    refresh_screen();
    delay(20000000);
    
    // Frame 2: Draw red rectangle
    clear_screen(BLACK);
    draw_rectangle(50, 50, 100, 80, RED);
    refresh_screen();
    delay(20000000);
    
    // Frame 3: Draw green rectangle
    clear_screen(BLACK);
    draw_rectangle(100, 80, 120, 60, GREEN);
    refresh_screen();
    delay(20000000);
    
    // Frame 4: Draw blue rectangle
    clear_screen(BLACK);
    draw_rectangle(150, 110, 80, 100, BLUE);
    refresh_screen();
    delay(20000000);
    
    // Frame 5: Draw multiple colored rectangles
    clear_screen(BLACK);
    draw_rectangle(10, 10, 60, 60, RED);
    draw_rectangle(80, 10, 60, 60, GREEN);
    draw_rectangle(150, 10, 60, 60, BLUE);
    refresh_screen();
    delay(50000000);
    
    // Keep window open longer
    delay(100000000);
    
    return 0;
}
