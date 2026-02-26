/**
 * @file example.c
 * @brief Example program for TLV320DAC3100 using Raspberry Pi Pico SDK
 * 
 * This example demonstrates how to initialize and configure the TLV320DAC3100
 * audio codec for I2S audio playback with headphone detection.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico_tlv320dac3100.h"

// Pin definitions (adjust these to match your hardware)
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define TLV_RESET_PIN 6
#define I2C_PORT i2c0
#define I2C_BAUDRATE (400 * 1000)  // 400 kHz

// Codec device
static tlv320dac3100_t codec;

void halt(const char *message) {
    printf("FATAL ERROR: %s\n", message);
    while (1) {
        tight_loop_contents();
    }
}

int main() {
    // Initialize stdio for USB output
    stdio_init_all();
    
    // Wait a bit for USB to enumerate
    sleep_ms(2000);
    printf("\n=== TLV320DAC3100 Example ===\n");
    
    // Initialize I2C
    printf("Initializing I2C...\n");
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Hardware reset via GPIO
    printf("Resetting codec...\n");
    gpio_init(TLV_RESET_PIN);
    gpio_set_dir(TLV_RESET_PIN, GPIO_OUT);
    gpio_put(TLV_RESET_PIN, 0);
    sleep_ms(100);
    gpio_put(TLV_RESET_PIN, 1);
    sleep_ms(10);
    
    // Initialize codec
    printf("Initializing TLV DAC...\n");
    if (!tlv320_init(&codec, I2C_PORT, TLV320DAC3100_I2CADDR_DEFAULT)) {
        halt("Failed to initialize codec!");
    }
    sleep_ms(10);
    
    // Interface Control
    printf("Configuring codec interface...\n");
    if (!tlv320_set_codec_interface(&codec, 
                                     TLV320DAC3100_FORMAT_I2S,      // Format: I2S
                                     TLV320DAC3100_DATA_LEN_16,     // Length: 16 bits
                                     false,                          // BCLK input
                                     false)) {                       // WCLK input
        halt("Failed to configure codec interface!");
    }
    
    // Clock MUX and PLL settings
    printf("Configuring clocks...\n");
    if (!tlv320_set_codec_clock_input(&codec, TLV320DAC3100_CODEC_CLKIN_PLL) ||
        !tlv320_set_pll_clock_input(&codec, TLV320DAC3100_PLL_CLKIN_BCLK)) {
        halt("Failed to configure codec clocks!");
    }
    
    // P=1, R=2, J=32, D=0 for typical BCLK-based PLL configuration
    printf("Configuring PLL...\n");
    if (!tlv320_set_pll_values(&codec, 1, 2, 32, 0)) {
        halt("Failed to configure PLL values!");
    }
    
    // DAC/ADC Config
    printf("Configuring DAC dividers...\n");
    if (!tlv320_set_ndac(&codec, true, 8) ||    // Enable NDAC with value 8
        !tlv320_set_mdac(&codec, true, 2)) {    // Enable MDAC with value 2
        halt("Failed to configure DAC dividers!");
    }
    
    printf("Powering up PLL...\n");
    if (!tlv320_power_pll(&codec, true)) {
        halt("Failed to power up PLL!");
    }
    
    // DAC Setup
    printf("Configuring DAC data path...\n");
    if (!tlv320_set_dac_data_path(&codec, 
                                   true, true,                      // Power up both DACs
                                   TLV320_DAC_PATH_NORMAL,         // Normal left path
                                   TLV320_DAC_PATH_NORMAL,         // Normal right path
                                   TLV320_VOLUME_STEP_1SAMPLE)) {  // Step: 1 per sample
        halt("Failed to configure DAC data path!");
    }
    
    printf("Configuring analog routing...\n");
    if (!tlv320_configure_analog_inputs(&codec, 
                                         TLV320_DAC_ROUTE_MIXER,    // Left DAC to mixer
                                         TLV320_DAC_ROUTE_MIXER,    // Right DAC to mixer
                                         false, false, false,        // No AIN routing
                                         false)) {                   // No HPL->HPR
        halt("Failed to configure DAC routing!");
    }
    
    // DAC Volume Control
    printf("Setting DAC volume...\n");
    if (!tlv320_set_dac_volume_control(&codec, 
                                        false, false,               // Unmute both channels
                                        TLV320_VOL_INDEPENDENT) ||
        !tlv320_set_channel_volume(&codec, false, 18.0f) ||        // Left DAC +18dB
        !tlv320_set_channel_volume(&codec, true, 18.0f)) {         // Right DAC +18dB
        halt("Failed to configure DAC volumes!");
    }
    
    // Headphone and Speaker Setup
    printf("Configuring headphone driver...\n");
    if (!tlv320_configure_headphone_driver(&codec,
                                            true, true,             // Power up both drivers
                                            TLV320_HP_COMMON_1_35V, // Default common mode
                                            false) ||               // Don't power down on SCD
        !tlv320_configure_hpl_pga(&codec, 0, true) ||              // Set HPL gain, unmute
        !tlv320_configure_hpr_pga(&codec, 0, true) ||              // Set HPR gain, unmute
        !tlv320_set_hpl_volume(&codec, true, 6) ||                 // Enable and set HPL volume
        !tlv320_set_hpr_volume(&codec, true, 6)) {                 // Enable and set HPR volume
        halt("Failed to configure headphone outputs!");
    }
    
    printf("Configuring speaker driver...\n");
    if (!tlv320_enable_speaker(&codec, true) ||                    // Enable speaker amp
        !tlv320_configure_spk_pga(&codec, TLV320_SPK_GAIN_6DB,    // Set gain to 6dB
                                   true) ||                         // Unmute
        !tlv320_set_spk_volume(&codec, true, 0)) {                 // Enable and set volume
        halt("Failed to configure speaker output!");
    }
    
    printf("Configuring headset detection...\n");
    if (!tlv320_config_micbias(&codec, false, true, TLV320_MICBIAS_AVDD) ||
        !tlv320_set_headset_detect(&codec, true, 
                                    TLV320_DEBOUNCE_16MS,
                                    TLV320_BTN_DEBOUNCE_0MS) ||
        !tlv320_set_int1_source(&codec, true, true, false, false, false, false) ||
        !tlv320_set_gpio1_mode(&codec, TLV320_GPIO1_INT1)) {
        halt("Failed to configure headset detect!");
    }
    
    printf("\n=== TLV320DAC3100 Configuration Complete ===\n");
    printf("Monitoring headset status...\n\n");
    
    // Main loop - monitor headset status
    tlv320_headset_status_t last_status = TLV320_HEADSET_NONE;
    
    while (1) {
        tlv320_headset_status_t status = tlv320_get_headset_status(&codec);
        
        if (last_status != status) {
            switch (status) {
                case TLV320_HEADSET_NONE:
                    printf("Headset removed\n");
                    break;
                case TLV320_HEADSET_WITHOUT_MIC:
                    printf("Headphones detected\n");
                    break;
                case TLV320_HEADSET_WITH_MIC:
                    printf("Headset with mic detected\n");
                    break;
            }
            last_status = status;
        }
        
        // Read the sticky IRQ flags
        uint8_t flags = tlv320_read_irq_flags(&codec, true);
        
        // Only print if there are flags set
        if (flags) {
            printf("IRQ Flags detected:\n");
            
            if (flags & TLV320DAC3100_IRQ_HPL_SHORT) {
                printf("  - Short circuit detected at HPL / left class-D driver\n");
            }
            
            if (flags & TLV320DAC3100_IRQ_HPR_SHORT) {
                printf("  - Short circuit detected at HPR / right class-D driver\n");
            }
            
            if (flags & TLV320DAC3100_IRQ_BUTTON_PRESS) {
                printf("  - Headset button pressed\n");
            }
            
            if (flags & TLV320DAC3100_IRQ_HEADSET_DETECT) {
                printf("  - Headset insertion/removal detected\n");
            }
            
            if (flags & TLV320DAC3100_IRQ_LEFT_DRC) {
                printf("  - Left DAC signal power greater than DRC threshold\n");
            }
            
            if (flags & TLV320DAC3100_IRQ_RIGHT_DRC) {
                printf("  - Right DAC signal power greater than DRC threshold\n");
            }
            
            printf("  Raw flag value: 0x%02X\n\n", flags);
        }
        
        sleep_ms(100);
    }
    
    return 0;
}
