# SSD1681 E-Paper Display Driver for Raspberry Pi Pico

Clean, direct Pico SDK implementation with no HAL abstraction.

## Location

All Pico implementation files are in the `/pico` directory.

## Features

- ✅ Direct Pico SDK integration (no abstraction layer)
- ✅ Both 3-wire and 4-wire SPI modes
- ✅ Runtime pin configuration via init
- ✅ Simple, clean API
- ✅ ~200×200 pixel display support

## Quick Start

```bash
cd pico

# Build for 4-wire SPI (default)
./build.sh

# Or build for 3-wire SPI
./build.sh 3wire

# Flash build/example.uf2 to Pico
```

## Pin Configuration

Pins are configured at initialization:

```c
ssd1681_config_t config;

// Get defaults (4-wire or 3-wire)
ssd1681_get_default_config_4wire(&config);

// Customize pins
config.pin_mosi = 19;
config.pin_sck = 18;
config.pin_cs = 17;
config.pin_dc = 20;     // Not used in 3-wire mode
config.pin_rst = 21;
config.pin_busy = 22;
config.spi_baudrate = 4000000;

// Initialize
ssd1681_init(&config);
```

## Wiring

### 4-Wire SPI (6 pins)
```
Display  →  Pico GPIO
─────────────────────
VCC      →  3V3
GND      →  GND
MOSI     →  19
SCK      →  18
CS       →  17
D/C      →  20
RST      →  21
BUSY     →  22
```

### 3-Wire SPI (5 pins - saves D/C!)
```
Display  →  Pico GPIO
─────────────────────
VCC      →  3V3
GND      →  GND
MOSI     →  19
SCK      →  18
CS       →  17
D/C      →  NOT CONNECTED
RST      →  21
BUSY     →  22
```

## API

### Initialize
```c
ssd1681_config_t config;
ssd1681_get_default_config_4wire(&config);  // or _3wire
ssd1681_init(&config);
```

### Draw
```c
ssd1681_clear(SSD1681_COLOR_BLACK);
ssd1681_write_point(SSD1681_COLOR_BLACK, x, y, 1);
ssd1681_fill_rect(SSD1681_COLOR_BLACK, x1, y1, x2, y2, 1);
ssd1681_draw_picture(SSD1681_COLOR_BLACK, x1, y1, x2, y2, img);
```

### Update
```c
ssd1681_update();  // Refresh display (~2.5 seconds)
```

## Documentation

- `/pico/README.md` - Full API reference
- `/pico/QUICKSTART.md` - Quick setup guide
- `/pico/example.c` - Example code

## Files

```
pico/
├── ssd1681_pico.h       - API header
├── ssd1681_pico.c       - Implementation
├── example.c            - Example app
├── CMakeLists.txt       - Build config
├── build.sh             - Build script
├── README.md            - Full docs
└── QUICKSTART.md        - Quick start
```

## Mode Selection

**Use 4-wire:**
- Universal compatibility
- Standard implementation

**Use 3-wire:**
- Save 1 GPIO pin
- Display must support 3-wire mode
- Check datasheet for BS1 pin

## License

MIT License - Copyright (c) 2026 OpenCode

---

**Note:** The `/src`, `/interface`, and `/example` directories contain the original libdriver implementation. The `/pico` implementation is standalone and does not require these files.
