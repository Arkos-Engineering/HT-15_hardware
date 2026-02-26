# TLV320DAC3100 Library for Raspberry Pi Pico SDK

This is a native Raspberry Pi Pico SDK port of the Adafruit TLV320DAC3100 library, removing all Arduino and external dependencies. 

DISCLAMER: This was re-written primarily by Claude however has been reviewed and maintained by humans

## Features

- **Pure Pico SDK Implementation**: No Arduino or Adafruit_BusIO dependencies
- **Complete I2S DAC Control**: Full access to all TLV320DAC3100 features
- **Hardware I2C**: Uses native Pico hardware I2C for efficient communication
- **Comprehensive API**: All original functionality preserved
  - PLL and clock configuration
  - I2S audio interface setup
  - DAC volume and routing control
  - Headphone and speaker amplifier configuration
  - Headset detection with button press support
  - Beep tone generator
  - GPIO and interrupt management

## Pin Connections

Default connections (can be customized):
```
Pico Pin    | TLV320 Pin | Function
------------|------------|----------
GPIO 4      | SDA        | I2C Data
GPIO 5      | SCL        | I2C Clock
GPIO 6      | RESET      | Hardware Reset
GND         | GND        | Ground
3.3V        | VIN        | Power
```

## Building

### Prerequisites

1. Install the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
2. Set the `PICO_SDK_PATH` environment variable:
   ```bash
   export PICO_SDK_PATH=/path/to/pico-sdk
   ```

### Build as a Library

Add this library to your project:

```cmake
# In your CMakeLists.txt
add_subdirectory(path/to/pico_tlv320_i2s)

target_link_libraries(your_project
    pico_stdlib
    hardware_i2c
    pico_tlv320dac3100
)
```

### Build the Example

```bash
mkdir build
cd build
cmake .. -DBUILD_EXAMPLES=ON
make
```

This will generate `tlv320_example.uf2` which can be flashed to your Pico.

## Usage Example

```c
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico_tlv320dac3100.h"

#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define TLV_RESET_PIN 6

int main() {
    stdio_init_all();
    
    // Initialize I2C
    i2c_init(i2c0, 400 * 1000);  // 400kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Hardware reset
    gpio_init(TLV_RESET_PIN);
    gpio_set_dir(TLV_RESET_PIN, GPIO_OUT);
    gpio_put(TLV_RESET_PIN, 0);
    sleep_ms(100);
    gpio_put(TLV_RESET_PIN, 1);
    
    // Initialize codec
    tlv320dac3100_t codec;
    if (!tlv320_init(&codec, i2c0, TLV320DAC3100_I2CADDR_DEFAULT)) {
        printf("Failed to initialize codec\n");
        return -1;
    }
    
    // Configure I2S interface (16-bit I2S format)
    tlv320_set_codec_interface(&codec, 
                                TLV320DAC3100_FORMAT_I2S,
                                TLV320DAC3100_DATA_LEN_16,
                                false, false);
    
    // Configure PLL (BCLK input, typical settings)
    tlv320_set_codec_clock_input(&codec, TLV320DAC3100_CODEC_CLKIN_PLL);
    tlv320_set_pll_clock_input(&codec, TLV320DAC3100_PLL_CLKIN_BCLK);
    tlv320_set_pll_values(&codec, 1, 2, 32, 0);
    tlv320_power_pll(&codec, true);
    
    // Configure DAC dividers
    tlv320_set_ndac(&codec, true, 8);
    tlv320_set_mdac(&codec, true, 2);
    
    // Enable DACs and set volume
    tlv320_set_dac_data_path(&codec, true, true,
                              TLV320_DAC_PATH_NORMAL,
                              TLV320_DAC_PATH_NORMAL,
                              TLV320_VOLUME_STEP_1SAMPLE);
    
    tlv320_set_channel_volume(&codec, false, 0.0f);  // Left 0dB
    tlv320_set_channel_volume(&codec, true, 0.0f);   // Right 0dB
    
    // Configure headphone output
    tlv320_configure_analog_inputs(&codec,
                                    TLV320_DAC_ROUTE_MIXER,
                                    TLV320_DAC_ROUTE_MIXER,
                                    false, false, false, false);
    
    tlv320_configure_headphone_driver(&codec, true, true,
                                       TLV320_HP_COMMON_1_35V, false);
    tlv320_configure_hpl_pga(&codec, 0, true);
    tlv320_configure_hpr_pga(&codec, 0, true);
    tlv320_set_hpl_volume(&codec, true, 0x7F);
    tlv320_set_hpr_volume(&codec, true, 0x7F);
    
    printf("TLV320DAC3100 initialized!\n");
    
    while (1) {
        // Your audio processing code here
        sleep_ms(100);
    }
    
    return 0;
}
```

## API Reference

### Initialization
- `tlv320_init()` - Initialize the codec
- `tlv320_reset()` - Software reset

### Clock Configuration
- `tlv320_set_pll_clock_input()` - Set PLL input source
- `tlv320_set_codec_clock_input()` - Set codec clock source
- `tlv320_set_pll_values()` - Configure PLL P, R, J, D values
- `tlv320_configure_pll()` - Auto-calculate PLL settings
- `tlv320_power_pll()` - Enable/disable PLL
- `tlv320_set_ndac()` / `tlv320_set_mdac()` / `tlv320_set_dosr()` - Configure dividers

### I2S Interface
- `tlv320_set_codec_interface()` - Configure I2S format, word length, BCLK/WCLK direction

### DAC Control
- `tlv320_set_dac_data_path()` - Power and routing control
- `tlv320_set_channel_volume()` - Set volume in dB (-63.5 to +24 dB)
- `tlv320_set_dac_volume_control()` - Mute and volume linking

### Output Drivers
- `tlv320_configure_headphone_driver()` - Configure headphone amplifier
- `tlv320_configure_hpl_pga()` / `tlv320_configure_hpr_pga()` - Set headphone gain
- `tlv320_enable_speaker()` - Enable class-D speaker amplifier
- `tlv320_configure_spk_pga()` - Set speaker gain

### Headset Detection
- `tlv320_set_headset_detect()` - Enable headset detection
- `tlv320_get_headset_status()` - Read headset status

### Interrupts
- `tlv320_set_int1_source()` / `tlv320_set_int2_source()` - Configure interrupt sources
- `tlv320_read_irq_flags()` - Read interrupt flags

See `pico_tlv320dac3100.h` for complete API documentation.

## Differences from Arduino Library

1. **No Arduino Dependencies**: Uses native Pico SDK I2C functions
2. **C-Style API**: Function-based API with device structure instead of C++ class
3. **Manual I2C Setup**: You must initialize I2C before calling `tlv320_init()`
4. **Function Naming**: Snake_case instead of camelCase (e.g., `tlv320_set_volume` instead of `setVolume`)
5. **No TwoWire**: Uses `i2c_inst_t*` directly (i2c0 or i2c1)

## Original Arduino Library

This is a port of the Adafruit TLV320DAC3100 library:
- Original Author: Limor Fried (ladyada) for Adafruit Industries
- Arduino Library: https://github.com/adafruit/Adafruit_TLV320_I2S
- Product: http://www.adafruit.com/product/6309

## License

MIT License

Copyright (c) 2024 Michael Bell and Pimoroni Ltd

Original library copyright Adafruit Industries

See LICENSE.txt for full license text.

## Credits

- **Original Library**: Written by Limor Fried for Adafruit Industries
- **Pico SDK Port**: Removed Arduino/Adafruit_BusIO dependencies, native Pico I2C implementation
- **Hardware**: Designed by Adafruit Industries

## Support

For issues specific to the Pico SDK port, please open an issue in this repository.

For questions about the TLV320DAC3100 chip itself or the hardware breakout board, see:
- [Adafruit Product Page](http://www.adafruit.com/product/6309)
- [TLV320DAC3100 Datasheet](https://www.ti.com/product/TLV320DAC3100)
