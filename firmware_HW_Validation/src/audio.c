/**
 * @file audio.h
 * @brief Controlls all audio functions for the HT-15
 * 
 */
#include "audio.h"
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"



void audio_init(audio_config_t *audio_config, uint8_t pin_amp_reset, uint8_t pin_master_clock, i2c_inst_t *i2c, uint8_t i2c_addr){
    //return pointer to global audio config struct
    audio_config->pin_amp_reset = pin_amp_reset;
    audio_config->pin_master_clock = pin_master_clock;

    // Setup reset pin and perform hard reset BEFORE initializing
    audio_amp_reset_hard(audio_config);

    // Initialize TLV320DAC3100 audio amplifier over I2C and init audio_config->audioamp
    tlv320_init(&audio_config->audioamp, i2c, i2c_addr);
    
    audio_start_system_clock(audio_config);
    
    // Set DAC processing block (PRB_P25 supports beep generator)
    tlv320_set_dac_processing_block(&audio_config->audioamp, 25);

    // Set PLL clock input to MCLK (12MHz from RP2350)
    tlv320_set_pll_clock_input(&audio_config->audioamp, TLV320DAC3100_PLL_CLKIN_MCLK);
    sleep_ms(10);
    // Set codec clock input to PLL (12MHz from RP2350)
    tlv320_set_codec_clock_input(&audio_config->audioamp, TLV320DAC3100_CODEC_CLKIN_PLL);

    // Set clock dividers for 16kHz sample rate
    // MCLK=12MHz, NDAC=3, MDAC=2, DOSR=125
    // DAC_CLK = MCLK / NDAC = 12MHz / 3 = 4MHz
    // DAC_MOD_CLK = DAC_CLK / MDAC = 4MHz / 2 = 2MHz  
    // DAC_FS = DAC_MOD_CLK / DOSR = 2MHz / 125 = 16kHz
    tlv320_set_ndac(&audio_config->audioamp, true, 2);
    tlv320_set_mdac(&audio_config->audioamp, true, 8);
    tlv320_set_dosr(&audio_config->audioamp, 128);
    tlv320_set_pll_values(&audio_config->audioamp, 1, 1, 8, 1920); // Set PLL to multiply MCLK by 4 (12MHz * 4 = 48MHz PLL clock)

    tlv320_power_pll(&audio_config->audioamp, true);

    // Configure codec interface - I2S, 16-bit, codec is master (bclk_out=true, wclk_out=true)
    // IMPORTANT: Must be set BEFORE configuring BCLK dividers per datasheet power-up sequence
    tlv320_set_codec_interface(&audio_config->audioamp, TLV320DAC3100_FORMAT_I2S, TLV320DAC3100_DATA_LEN_16, true, true);

    // Configure BCLK generation:
    tlv320_set_bclk_n(&audio_config->audioamp, true, 1);  // Enable BCLK divider with N=1
    
    // Configure BCLK output behavior (Page 1, Reg 0x1D):
    // - invert_bclk=false: normal polarity
    // - active_when_powered_down=true: keep BCLK running 
    // - source=DAC_MOD_CLK: derive from DAC_MOD_CLK (2MHz) for cleaner division
    tlv320_set_bclk_config(&audio_config->audioamp, false, true, TLV320DAC3100_BCLK_SRC_DAC_MOD_CLK);

    // Configure timer/delay clock divider (Page 3, Reg 16)
    // For 12MHz MCLK, divider of 12 gives ~1MHz timer clock for beep generator
    // Per TLV320DAC3100 datasheet Section 7.4.3
    tlv320_config_delay_divider(&audio_config->audioamp, true, 12);

    // Route DAC to speaker mixer (not headphone)
    tlv320_configure_analog_inputs(&audio_config->audioamp, TLV320_DAC_ROUTE_MIXER, TLV320_DAC_ROUTE_MIXER, false, false, false, false);
    
    sleep_ms(50);

    // Power on DAC
    // Enable DAC data path - both channels on, normal routing
    tlv320_set_dac_data_path(&audio_config->audioamp, true, true, TLV320_DAC_PATH_NORMAL, TLV320_DAC_PATH_NORMAL, TLV320_VOLUME_STEP_2SAMPLE);

    // Unmute DAC channels
    tlv320_set_dac_volume_control(&audio_config->audioamp, true, true, TLV320_VOL_RIGHT_TO_LEFT);
    
    // Set DAC digital volume (0dB for left and right channels)
    tlv320_set_channel_volume(&audio_config->audioamp, false, 0);  // Left channel 0dB
    tlv320_set_channel_volume(&audio_config->audioamp, true, 0);   // Right channel 0dB

    // Enable speaker amplifier
    tlv320_enable_speaker(&audio_config->audioamp, true);
    
    // Configure speaker driver - gain and unmute
    // TLV320_SPK_GAIN_6DB = +6dB gain stage
    tlv320_configure_spk_pga(&audio_config->audioamp, TLV320_SPK_GAIN_6DB, true);
    
    // Set speaker analog volume - route enabled, 0dB gain
    tlv320_set_spk_volume(&audio_config->audioamp, true, 0);
    
    // Wait for output drivers to stabilize
    sleep_ms(50);
}

void audio_amp_reset_hard(const audio_config_t *audio_config){
    gpio_init(audio_config->pin_amp_reset);
    gpio_set_dir(audio_config->pin_amp_reset, GPIO_OUT);
    gpio_put(audio_config->pin_amp_reset, 0); //start_reset
    sleep_us(10);
    gpio_put(audio_config->pin_amp_reset, 1); //finish_reset
}

void audio_start_system_clock(const audio_config_t *audio_config){
    //configure microcontroller to output required MCLK signal on pin specified by master_clock_pin, derived from system clock
    //set clkout to the crystal oscillator frequency (12MHz)
    clock_gpio_init(audio_config->pin_master_clock, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 1);
}

void audio_stop_system_clock(const audio_config_t *audio_config){
    //disable MCLK output
    gpio_init(audio_config->pin_master_clock);              // makes it a normal GPIO again
    gpio_set_dir(audio_config->pin_master_clock, GPIO_OUT);
    gpio_put(audio_config->pin_master_clock, 0);
}

void audio_amp_set_volume(const audio_config_t *audio_config, uint8_t volume){
    //set volume on audio amp (0-99)
    //map 0-99 to -61 to 0 dB
    float vol_db = ((float)volume * 0.619191) - 61.0f;
    tlv320_set_channel_volume(&audio_config->audioamp, false, (float)vol_db);  // Left channel
    tlv320_set_channel_volume(&audio_config->audioamp, true, (float)vol_db);   // Right channel
}


/// @brief Generate a beep tone on the speaker
/// @param audio_config Pointer to the audio configuration struct
/// @param frequency_hz Frequency of the beep in Hz (recommended 500-2000Hz)
/// @param duration_ms Duration of the beep in milliseconds
/// @param volume_db Volume in dB (0 = max, -61 = min)
void audio_beep(const audio_config_t *audio_config, uint16_t frequency_hz, uint16_t duration_ms, int8_t volume_db){
    // Sample rate is 48kHz based on our clock divider configuration
    const uint32_t sample_rate = 48000;
    
    // Frequency must be less than sample_rate/4 per datasheet
    if (frequency_hz >= sample_rate / 4) {
        frequency_hz = sample_rate / 4 - 1;
    }
    
    // Configure the beep tone (sin/cos coefficients and length)
    // This calculates the proper phase increment for the DDS
    if (!tlv320_configure_beep_tone(&audio_config->audioamp, (float)frequency_hz, duration_ms, sample_rate)) {
        printf("Failed to configure beep tone\n");
        return;
    }
    
    // Set beep volume (0dB max, -61dB min)
    // Using same volume for both channels
    tlv320_set_beep_volume(&audio_config->audioamp, volume_db, volume_db);
    
    // Start the beep
    tlv320_enable_beep(&audio_config->audioamp, true);
    
    // printf("Beep: %dHz, %dms, %ddB\n", frequency_hz, duration_ms, volume_db);
}