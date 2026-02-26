# Quick Start Guide

## Setup

1. **Set Pico SDK path:**
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

2. **Build for 4-wire SPI (default):**
```bash
cd pico
./build.sh
```

Or **build for 3-wire SPI:**
```bash
./build.sh 3wire
```

3. **Flash to Pico:**
- Hold BOOTSEL button
- Connect USB
- Copy `build/example.uf2` to RPI-RP2 drive

## Wiring

### 4-Wire SPI (6 connections)
```
Display  →  Pico
VCC      →  3V3
GND      →  GND
DIN      →  GPIO 19 (MOSI)
CLK      →  GPIO 18 (SCK)
CS       →  GPIO 17
DC       →  GPIO 20
RST      →  GPIO 21
BUSY     →  GPIO 22
```

### 3-Wire SPI (5 connections - saves D/C pin!)
```
Display  →  Pico
VCC      →  3V3
GND      →  GND
DIN      →  GPIO 19 (MOSI)
CLK      →  GPIO 18 (SCK)
CS       →  GPIO 17
DC       →  NOT CONNECTED
RST      →  GPIO 21
BUSY     →  GPIO 22
```

## Custom Pins

Edit your code:

```c
ssd1681_config_t config;
ssd1681_get_default_config_4wire(&config);

// Customize
config.pin_mosi = 3;
config.pin_sck = 2;
config.pin_cs = 5;
config.spi_baudrate = 8000000;  // 8 MHz

ssd1681_init(&config);
```

## Minimal Example

```c
#include "ssd1681_pico.h"

int main(void) {
    ssd1681_config_t config;
    
    ssd1681_get_default_config_4wire(&config);
    ssd1681_init(&config);
    
    ssd1681_clear(SSD1681_COLOR_BLACK);
    ssd1681_fill_rect(SSD1681_COLOR_BLACK, 50, 50, 150, 100, 1);
    ssd1681_update();
    
    while(1);
}
```

## Mode Selection

**Use 4-wire when:**
- Maximum compatibility needed
- Display mode unknown

**Use 3-wire when:**
- Need to save a GPIO pin
- Display confirmed to support 3-wire
- Check datasheet for "3-wire SPI" or "BS1 pin"
