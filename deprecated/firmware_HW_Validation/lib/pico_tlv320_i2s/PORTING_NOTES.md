# Pico SDK Port - Technical Notes

## Overview

This document describes the changes made to port the Adafruit TLV320DAC3100 library from Arduino to the Raspberry Pi Pico SDK.

## Files Created

### New Pico SDK Files
1. **pico_tlv320dac3100.h** - Header file with all type definitions and function declarations
2. **pico_tlv320dac3100.c** - Implementation using native Pico SDK I2C functions
3. **CMakeLists.txt** - Build configuration for Pico SDK
4. **example.c** - Example program demonstrating usage
5. **README_PICO.md** - Documentation for Pico SDK users

### Original Arduino Files (Preserved)
- `Adafruit_TLV320DAC3100.h` - Original Arduino header
- `Adafruit_TLV320DAC3100.cpp` - Original Arduino implementation
- `Adafruit_TLV320DAC3100_typedefs.h` - Original type definitions
- `examples/basicI2Sconfig/basicI2Sconfig.ino` - Original Arduino example

## Key Changes

### 1. I2C Communication Layer

**Arduino (Adafruit_BusIO):**
```cpp
Adafruit_I2CDevice *i2c_dev = new Adafruit_I2CDevice(address, wire);
Adafruit_BusIO_Register reg(i2c_dev, TLV320DAC3100_REG_RESET);
Adafruit_BusIO_RegisterBits bits(&reg, 1, 0);
bits.write(value);
```

**Pico SDK:**
```c
uint8_t data[2] = {reg_addr, value};
i2c_write_blocking(dev->i2c, dev->addr, data, 2, false);
```

### 2. Device Structure

**Arduino (C++ Class):**
```cpp
class Adafruit_TLV320DAC3100 {
private:
    Adafruit_I2CDevice *i2c_dev;
public:
    bool begin(uint8_t addr, TwoWire *wire);
    bool setVolume(float dB);
};
```

**Pico SDK (C Structure):**
```c
typedef struct {
    i2c_inst_t *i2c;
    uint8_t addr;
    uint8_t current_page;
} tlv320dac3100_t;

bool tlv320_init(tlv320dac3100_t *dev, i2c_inst_t *i2c, uint8_t addr);
bool tlv320_set_volume(tlv320dac3100_t *dev, float dB);
```

### 3. Register Access Abstraction

Created helper functions to replace Adafruit_BusIO functionality:

```c
static bool write_register(tlv320dac3100_t *dev, uint8_t reg, uint8_t value);
static bool read_register(tlv320dac3100_t *dev, uint8_t reg, uint8_t *value);
static bool write_bits(tlv320dac3100_t *dev, uint8_t reg, uint8_t shift, 
                        uint8_t width, uint8_t value);
static bool read_bits(tlv320dac3100_t *dev, uint8_t reg, uint8_t shift, 
                       uint8_t width, uint8_t *value);
```

### 4. Bit Manipulation Macros

```c
#define BIT_MASK(width) ((1u << (width)) - 1)
#define GET_BITS(val, shift, width) (((val) >> (shift)) & BIT_MASK(width))
#define SET_BITS(val, bits, shift, width) \
    (((val) & ~(BIT_MASK(width) << (shift))) | \
     (((bits) & BIT_MASK(width)) << (shift)))
```

### 5. Delay Functions

**Arduino:**
```cpp
delay(10);  // milliseconds
```

**Pico SDK:**
```c
sleep_ms(10);  // milliseconds
```

### 6. Function Naming Convention

Changed from camelCase to snake_case:
- `setChannelVolume()` → `tlv320_set_channel_volume()`
- `isPLLpowered()` → `tlv320_is_pll_powered()`
- `configureHeadphoneDriver()` → `tlv320_configure_headphone_driver()`

### 7. Math Functions

**Arduino:**
```cpp
#include <Arduino.h>  // Includes math.h
float angle = 2.0 * PI * frequency / sample_rate;
```

**Pico SDK:**
```c
#include <math.h>
float angle = 2.0f * M_PI * frequency / sample_rate;
```

### 8. Type Definitions

All typedef enums were preserved from `Adafruit_TLV320DAC3100_typedefs.h` and moved to the new header file.

### 9. Page Select Optimization

Added page caching to reduce unnecessary I2C transactions:

```c
static bool set_page(tlv320dac3100_t *dev, uint8_t page) {
    if (dev->current_page == page) {
        return true;  // Already on correct page
    }
    // Only switch if needed
    uint8_t data[2] = {TLV320DAC3100_REG_PAGE_SELECT, page};
    int result = i2c_write_blocking(dev->i2c, dev->addr, data, 2, false);
    if (result == 2) {
        dev->current_page = page;
        return true;
    }
    return false;
}
```

## Feature Parity

All features from the original Arduino library have been ported:

✅ Clock and PLL configuration  
✅ I2S codec interface setup  
✅ DAC data path and volume control  
✅ NDAC, MDAC, DOSR divider configuration  
✅ Headphone amplifier configuration  
✅ Speaker amplifier configuration  
✅ Headset detection  
✅ Beep tone generator  
✅ GPIO and interrupt control  
✅ Volume ADC  
✅ MICBIAS configuration  
✅ Flag and status reading  

## Dependencies Removed

1. **Arduino.h** - Replaced with pico/stdlib.h
2. **Wire.h** - Replaced with hardware/i2c.h
3. **Adafruit_I2CDevice** - Implemented using i2c_write_blocking/i2c_read_blocking
4. **Adafruit_BusIO_Register** - Implemented custom register read/write helpers
5. **Adafruit_BusIO_RegisterBits** - Implemented custom bit manipulation helpers

## Dependencies Added

1. **pico/stdlib.h** - Core Pico SDK functions
2. **hardware/i2c.h** - Hardware I2C functions
3. **math.h** - Standard C math library (for PLL and beep calculations)

## Testing Recommendations

1. **I2C Communication**: Verify I2C transactions with logic analyzer
2. **PLL Configuration**: Test audio output at different sample rates
3. **Volume Control**: Verify volume changes work correctly
4. **Headset Detection**: Test headphone insertion/removal
5. **Interrupts**: Verify GPIO1 interrupt generation
6. **Beep Generator**: Test tone generation at various frequencies

## Build Instructions

```bash
# Set Pico SDK path
export PICO_SDK_PATH=/path/to/pico-sdk

# Create build directory
mkdir build && cd build

# Configure with examples
cmake .. -DBUILD_EXAMPLES=ON

# Build
make

# Flash to Pico
# Copy build/tlv320_example.uf2 to Pico in BOOTSEL mode
```

## Migration Guide for Existing Arduino Code

### 1. Update includes
```c
// Before (Arduino)
#include <Adafruit_TLV320DAC3100.h>

// After (Pico SDK)
#include "pico_tlv320dac3100.h"
```

### 2. Initialize I2C manually
```c
// Pico SDK requires explicit I2C setup
i2c_init(i2c0, 400000);
gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
gpio_pull_up(I2C_SDA_PIN);
gpio_pull_up(I2C_SCL_PIN);
```

### 3. Change object creation
```c
// Before (Arduino)
Adafruit_TLV320DAC3100 codec;
codec.begin();

// After (Pico SDK)
tlv320dac3100_t codec;
tlv320_init(&codec, i2c0, TLV320DAC3100_I2CADDR_DEFAULT);
```

### 4. Update function calls
```c
// Before (Arduino)
codec.setChannelVolume(false, 0.0);

// After (Pico SDK)
tlv320_set_channel_volume(&codec, false, 0.0f);
```

## Performance Considerations

1. **I2C Speed**: Default 400kHz, can be adjusted via `i2c_init()`
2. **Page Caching**: Reduces I2C transactions when accessing same page
3. **Blocking I2C**: Uses `i2c_write_blocking()` - consider adding timeout handling
4. **No Dynamic Allocation**: All structures are stack-allocated

## Known Limitations

1. Only supports one codec instance (can be extended for multiple devices)
2. Blocking I2C operations (no async/DMA support)
3. No built-in error recovery for I2C bus hangs

## Future Enhancements

- [ ] Add DMA support for I2C transfers
- [ ] Add I2C bus recovery on communication failure
- [ ] Add support for multiple codec instances
- [ ] Add I2S audio streaming examples
- [ ] Add power management optimizations

## License

This port maintains the MIT license of the original Adafruit library.

## Credits

- **Original Library**: Limor Fried (Adafruit Industries)
- **Pico SDK Port**: Removed Arduino dependencies, native I2C implementation
- **Copyright**: Michael Bell and Pimoroni Ltd (2024)
