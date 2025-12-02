// tlv320aic3100.h
// Header file for TLV320AIC3100 audio amplifier control via I2C
#ifndef TLV320AIC3100_H
#define TLV320AIC3100_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <vector>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "pindefs.cpp"


class TLV320AIC3100{
    public:
        void init(i2c_inst_t *i2c_bus, int i2c_address, uint8_t reset_gpio_pin); // Initialize TLV320AIC3100 Audio Amplifier
        void reset(); // Reset the audio amplifier
        void reset_soft(); // Reset the audio amplifier through I2C command
        void reinit(); // Re-initialize the audio amplifier after power cycle

        void set_volume(uint8_t volume); // Set output volume (0-100)

        void beep(uint8_t volume);// Generate a beep sound at specified volume (0-100)

        void set_mute(bool mute_l, bool mute_r); // Mute or unmute audio output

        void set_dac_power(bool pow_l, bool pow_r); // Set DAC power state
        void set_speaker_amplifier_power(bool power_on); // Set overall amplifier power state

        bool write_page(uint8_t page); // Set register page
        bool write_register(uint8_t reg_address, uint8_t value); // Write value to register
        uint8_t read_register(uint8_t reg_address); // Read value from register

    private:
        i2c_inst_t *control_bus; //I2C bus instance
        uint8_t control_address; //7 bit I2C address
        uint8_t current_page; //current register page
        uint8_t reset_pin; //GPIO pin for hardware reset
        uint8_t volume; //volume for beep sound


};



#endif // TLV320AIC3100_H