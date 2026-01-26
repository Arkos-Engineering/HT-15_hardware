/**
 * @file audio.h
 * @brief Controlls all audio functions for the HT-15
 * 
 */
#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include "hardware/i2c.h"
#include "pico_tlv320dac3100.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    tlv320dac3100_t audioamp;
    uint8_t pin_amp_reset;
    uint8_t pin_master_clock;
} audio_config_t;

// Initialization and reset
void audio_init(audio_config_t *audio_config, uint8_t pin_amp_reset, uint8_t pin_master_clock, i2c_inst_t *i2c, uint8_t i2c_addr);
void audio_amp_reset_hard(const audio_config_t *audio_config);

// Clock control
void audio_start_system_clock(const audio_config_t *audio_config);
void audio_stop_system_clock(const audio_config_t *audio_config);

// Volume and playback
void audio_amp_set_volume(const audio_config_t *audio_config, uint8_t volume);
void audio_beep(const audio_config_t *audio_config, uint16_t frequency_hz, uint16_t duration_ms, int8_t volume_db);


#ifdef __cplusplus
}   
#endif

#endif // AUDIO_H
