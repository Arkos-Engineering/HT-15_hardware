# SSD1681 E-Paper Display Driver for Raspberry Pi Pico

Clean, direct Pico SDK implementation with no HAL abstraction layer.

## Features

- Direct Pico SDK integration
- Supports both 3-wire and 4-wire SPI modes
- Runtime pin configuration
- Simple API
- ~200×200 pixel display support

## Pin Configuration

Both 3-wire and 4-wire modes are configured at initialization:

```c
ssd1681_config_t config;

// 4-wire mode (default pins)
ssd1681_get_default_config_4wire(&config);
// MOSI: GPIO 19, SCK: GPIO 18, CS: GPIO 17
// D/C: GPIO 20, RST: GPIO 21, BUSY: GPIO 22

// 3-wire mode (default pins)
ssd1681_get_default_config_3wire(&config);
// MOSI: GPIO 19, SCK: GPIO 18, CS: GPIO 17
// RST: GPIO 21, BUSY: GPIO 22
// (No D/C pin - uses 9-bit SPI frames)

// Customize pins
config.pin_mosi = 3;
config.pin_sck = 2;
config.spi_baudrate = 8000000;  // 8 MHz

ssd1681_init(&config);
```

## Building

### 4-Wire SPI Mode (default)
```bash
mkdir build && cd build
cmake ..
make
```

### 3-Wire SPI Mode
```bash
mkdir build && cd build
cmake -DUSE_3WIRE_SPI=ON ..
make
```

## Usage Example

```c
#include "ssd1681_pico.h"

int main(void) {
    ssd1681_config_t config;
    
    // Get default 4-wire config
    ssd1681_get_default_config_4wire(&config);
    
    // Initialize
    ssd1681_init(&config);
    
    // Clear display
    ssd1681_clear(SSD1681_COLOR_BLACK);
    ssd1681_clear(SSD1681_COLOR_RED);
    
    // Draw rectangle
    ssd1681_fill_rect(SSD1681_COLOR_BLACK, 20, 20, 100, 80, 1);
    
    // Draw points
    ssd1681_write_point(SSD1681_COLOR_RED, 50, 50, 1);
    
    // Update display
    ssd1681_update();
    
    while(1) tight_loop_contents();
}
```

## API Reference

### Initialization
- `ssd1681_get_default_config_4wire()` - Get 4-wire defaults
- `ssd1681_get_default_config_3wire()` - Get 3-wire defaults
- `ssd1681_init()` - Initialize display with config
- `ssd1681_deinit()` - Deinitialize display

### Display Control
- `ssd1681_clear()` - Clear color plane
- `ssd1681_update()` - Refresh display

### Drawing
- `ssd1681_write_point()` - Draw single pixel
- `ssd1681_read_point()` - Read pixel value
- `ssd1681_fill_rect()` - Fill rectangle
- `ssd1681_draw_picture()` - Draw image buffer
- `ssd1681_draw_string()` - Draw text (requires font data)

## Pin Modes

### 4-Wire SPI
- Uses separate D/C pin
- Standard 8-bit SPI
- Universal compatibility

### 3-Wire SPI
- No D/C pin needed (saves 1 GPIO)
- Uses 9-bit SPI frames
- Display must support 3-wire mode

## Files

- `ssd1681_pico.h` - API header
- `ssd1681_pico.c` - Implementation
- `example.c` - Example application
- `CMakeLists.txt` - Build configuration

## License

MIT License - Copyright (c) 2026 OpenCode
