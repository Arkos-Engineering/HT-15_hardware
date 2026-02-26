/**
 * @file pico_tlv320dac3100.h
 * @brief Raspberry Pi Pico SDK library for TI TLV320DAC3100 stereo DAC
 *
 * This is a port of the Adafruit TLV320DAC3100 library to native Pico SDK,
 * removing all Arduino and external dependencies.
 *
 * Original library by Limor Fried for Adafruit Industries.
 * Ported to Pico SDK by removing Arduino/Adafruit_BusIO dependencies.
 *
 * MIT License - see LICENSE.txt for details
 * Copyright (c) 2024 Michael Bell and Pimoroni Ltd
 */

#ifndef _PICO_TLV320DAC3100_H
#define _PICO_TLV320DAC3100_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2C Address
#define TLV320DAC3100_I2CADDR_DEFAULT 0x18

// Page 0 Registers
#define TLV320DAC3100_REG_PAGE_SELECT 0x00
#define TLV320DAC3100_REG_RESET 0x01
#define TLV320DAC3100_REG_OT_FLAG 0x03
#define TLV320DAC3100_REG_CLOCK_MUX1 0x04
#define TLV320DAC3100_REG_PLL_PROG_PR 0x05
#define TLV320DAC3100_REG_PLL_PROG_J 0x06
#define TLV320DAC3100_REG_PLL_PROG_D_MSB 0x07
#define TLV320DAC3100_REG_PLL_PROG_D_LSB 0x08
#define TLV320DAC3100_REG_NDAC 0x0B
#define TLV320DAC3100_REG_MDAC 0x0C
#define TLV320DAC3100_REG_DOSR_MSB 0x0D
#define TLV320DAC3100_REG_DOSR_LSB 0x0E
#define TLV320DAC3100_REG_CLKOUT_MUX 0x19
#define TLV320DAC3100_REG_CLKOUT_M 0x1A
#define TLV320DAC3100_REG_CODEC_IF_CTRL1 0x1B
#define TLV320DAC3100_REG_DATA_SLOT_OFFSET 0x1C
#define TLV320DAC3100_REG_BCLK_CTRL2 0x1D
#define TLV320DAC3100_REG_BCLK_N 0x1E
#define TLV320DAC3100_REG_DAC_FLAG 0x25
#define TLV320DAC3100_REG_DAC_FLAG2 0x26
#define TLV320DAC3100_REG_INT1_CTRL 0x30
#define TLV320DAC3100_REG_INT2_CTRL 0x31
#define TLV320DAC3100_REG_GPIO1_CTRL 0x33
#define TLV320DAC3100_REG_DIN_CTRL 0x36
#define TLV320DAC3100_REG_DAC_PRB 0x3C
#define TLV320DAC3100_REG_DAC_DATAPATH 0x3F
#define TLV320DAC3100_REG_DAC_VOL_CTRL 0x40
#define TLV320DAC3100_REG_DAC_LVOL 0x41
#define TLV320DAC3100_REG_DAC_RVOL 0x42
#define TLV320DAC3100_REG_HEADSET_DETECT 0x43
#define TLV320DAC3100_REG_BEEP_L 0x47
#define TLV320DAC3100_REG_BEEP_R 0x48
#define TLV320DAC3100_REG_BEEP_LEN_MSB 0x49
#define TLV320DAC3100_REG_BEEP_LEN_MID 0x4A
#define TLV320DAC3100_REG_BEEP_LEN_LSB 0x4B
#define TLV320DAC3100_REG_BEEP_SIN_MSB 0x4C
#define TLV320DAC3100_REG_BEEP_SIN_LSB 0x4D
#define TLV320DAC3100_REG_BEEP_COS_MSB 0x4E
#define TLV320DAC3100_REG_BEEP_COS_LSB 0x4F
#define TLV320DAC3100_REG_VOL_ADC_CTRL 0x74
#define TLV320DAC3100_REG_VOL_ADC_READ 0x75

// Page 1 Registers
#define TLV320DAC3100_REG_HP_SPK_ERR_CTL 0x1E
#define TLV320DAC3100_REG_HP_DRIVERS 0x1F
#define TLV320DAC3100_REG_SPK_AMP 0x20
#define TLV320DAC3100_REG_HP_POP 0x21
#define TLV320DAC3100_REG_PGA_RAMP 0x22
#define TLV320DAC3100_REG_OUT_ROUTING 0x23
#define TLV320DAC3100_REG_HPL_VOL 0x24
#define TLV320DAC3100_REG_HPR_VOL 0x25
#define TLV320DAC3100_REG_SPK_VOL 0x26
#define TLV320DAC3100_REG_HPL_DRIVER 0x28
#define TLV320DAC3100_REG_HPR_DRIVER 0x29
#define TLV320DAC3100_REG_SPK_DRIVER 0x2A
#define TLV320DAC3100_REG_HP_DRIVER_CTRL 0x2C
#define TLV320DAC3100_REG_MICBIAS 0x2E
#define TLV320DAC3100_REG_INPUT_CM 0x32
#define TLV320DAC3100_REG_IRQ_FLAGS_STICKY 0x2C
#define TLV320DAC3100_REG_IRQ_FLAGS 0x2E

// Page 3 Registers
#define TLV320DAC3100_REG_TIMER_MCLK_DIV 0x10

// IRQ Flag bits
#define TLV320DAC3100_IRQ_HPL_SHORT 0x80
#define TLV320DAC3100_IRQ_HPR_SHORT 0x40
#define TLV320DAC3100_IRQ_BUTTON_PRESS 0x20
#define TLV320DAC3100_IRQ_HEADSET_DETECT 0x10
#define TLV320DAC3100_IRQ_LEFT_DRC 0x08
#define TLV320DAC3100_IRQ_RIGHT_DRC 0x04

// Type definitions
typedef enum {
    TLV320_DEBOUNCE_16MS = 0b000,
    TLV320_DEBOUNCE_32MS = 0b001,
    TLV320_DEBOUNCE_64MS = 0b010,
    TLV320_DEBOUNCE_128MS = 0b011,
    TLV320_DEBOUNCE_256MS = 0b100,
    TLV320_DEBOUNCE_512MS = 0b101,
} tlv320_detect_debounce_t;

typedef enum {
    TLV320_BTN_DEBOUNCE_0MS = 0b00,
    TLV320_BTN_DEBOUNCE_8MS = 0b01,
    TLV320_BTN_DEBOUNCE_16MS = 0b10,
    TLV320_BTN_DEBOUNCE_32MS = 0b11,
} tlv320_button_debounce_t;

typedef enum {
    TLV320_HEADSET_NONE = 0b00,
    TLV320_HEADSET_WITHOUT_MIC = 0b01,
    TLV320_HEADSET_WITH_MIC = 0b11,
} tlv320_headset_status_t;

typedef enum {
    TLV320_DAC_PATH_OFF = 0b00,
    TLV320_DAC_PATH_NORMAL = 0b01,
    TLV320_DAC_PATH_SWAPPED = 0b10,
    TLV320_DAC_PATH_MIXED = 0b11,
} tlv320_dac_path_t;

typedef enum {
    TLV320_VOLUME_STEP_1SAMPLE = 0b00,
    TLV320_VOLUME_STEP_2SAMPLE = 0b01,
    TLV320_VOLUME_STEP_DISABLED = 0b10,
} tlv320_volume_step_t;

typedef enum {
    TLV320_VOL_INDEPENDENT = 0b00,
    TLV320_VOL_LEFT_TO_RIGHT = 0b01,
    TLV320_VOL_RIGHT_TO_LEFT = 0b10,
} tlv320_vol_control_t;

typedef enum {
    TLV320DAC3100_CODEC_CLKIN_MCLK = 0b00,
    TLV320DAC3100_CODEC_CLKIN_BCLK = 0b01,
    TLV320DAC3100_CODEC_CLKIN_GPIO1 = 0b10,
    TLV320DAC3100_CODEC_CLKIN_PLL = 0b11,
} tlv320dac3100_codec_clkin_t;

typedef enum {
    TLV320DAC3100_PLL_CLKIN_MCLK = 0b00,
    TLV320DAC3100_PLL_CLKIN_BCLK = 0b01,
    TLV320DAC3100_PLL_CLKIN_GPIO1 = 0b10,
    TLV320DAC3100_PLL_CLKIN_DIN = 0b11
} tlv320dac3100_pll_clkin_t;

typedef enum {
    TLV320DAC3100_CDIV_CLKIN_MCLK = 0b000,
    TLV320DAC3100_CDIV_CLKIN_BCLK = 0b001,
    TLV320DAC3100_CDIV_CLKIN_DIN = 0b010,
    TLV320DAC3100_CDIV_CLKIN_PLL = 0b011,
    TLV320DAC3100_CDIV_CLKIN_DAC = 0b100,
    TLV320DAC3100_CDIV_CLKIN_DAC_MOD = 0b101,
} tlv320dac3100_cdiv_clkin_t;

typedef enum {
    TLV320DAC3100_DATA_LEN_16 = 0b00,
    TLV320DAC3100_DATA_LEN_20 = 0b01,
    TLV320DAC3100_DATA_LEN_24 = 0b10,
    TLV320DAC3100_DATA_LEN_32 = 0b11,
} tlv320dac3100_data_len_t;

typedef enum {
    TLV320DAC3100_FORMAT_I2S = 0b00,
    TLV320DAC3100_FORMAT_DSP = 0b01,
    TLV320DAC3100_FORMAT_RJF = 0b10,
    TLV320DAC3100_FORMAT_LJF = 0b11,
} tlv320dac3100_format_t;

typedef enum {
    TLV320_GPIO1_DISABLED = 0b0000,
    TLV320_GPIO1_INPUT_MODE = 0b0001,
    TLV320_GPIO1_GPI = 0b0010,
    TLV320_GPIO1_GPO = 0b0011,
    TLV320_GPIO1_CLKOUT = 0b0100,
    TLV320_GPIO1_INT1 = 0b0101,
    TLV320_GPIO1_INT2 = 0b0110,
    TLV320_GPIO1_BCLK_OUT = 0b1000,
    TLV320_GPIO1_WCLK_OUT = 0b1001,
} tlv320_gpio1_mode_t;

typedef enum {
    TLV320_DIN_DISABLED = 0b00,
    TLV320_DIN_ENABLED = 0b01,
    TLV320_DIN_GPI = 0b10,
} tlv320_din_mode_t;

typedef enum {
    TLV320_VOL_HYST_NONE = 0b00,
    TLV320_VOL_HYST_1BIT = 0b01,
    TLV320_VOL_HYST_2BIT = 0b10,
} tlv320_vol_hyst_t;

typedef enum {
    TLV320_VOL_RATE_15_625HZ = 0b000,
    TLV320_VOL_RATE_31_25HZ = 0b001,
    TLV320_VOL_RATE_62_5HZ = 0b010,
    TLV320_VOL_RATE_125HZ = 0b011,
    TLV320_VOL_RATE_250HZ = 0b100,
    TLV320_VOL_RATE_500HZ = 0b101,
    TLV320_VOL_RATE_1KHZ = 0b110,
    TLV320_VOL_RATE_2KHZ = 0b111,
} tlv320_vol_rate_t;

typedef enum {
    TLV320_HP_COMMON_1_35V = 0b00,
    TLV320_HP_COMMON_1_50V = 0b01,
    TLV320_HP_COMMON_1_65V = 0b10,
    TLV320_HP_COMMON_1_80V = 0b11,
} tlv320_hp_common_t;

typedef enum {
    TLV320_HP_TIME_0US = 0b0000,
    TLV320_HP_TIME_15US = 0b0001,
    TLV320_HP_TIME_153US = 0b0010,
    TLV320_HP_TIME_1_5MS = 0b0011,
    TLV320_HP_TIME_15MS = 0b0100,
    TLV320_HP_TIME_76MS = 0b0101,
    TLV320_HP_TIME_153MS = 0b0110,
    TLV320_HP_TIME_304MS = 0b0111,
    TLV320_HP_TIME_610MS = 0b1000,
    TLV320_HP_TIME_1_2S = 0b1001,
    TLV320_HP_TIME_3S = 0b1010,
    TLV320_HP_TIME_6S = 0b1011,
} tlv320_hp_time_t;

typedef enum {
    TLV320_RAMP_0MS = 0b00,
    TLV320_RAMP_1MS = 0b01,
    TLV320_RAMP_2MS = 0b10,
    TLV320_RAMP_4MS = 0b11,
} tlv320_ramp_time_t;

typedef enum {
    TLV320_SPK_WAIT_0MS = 0b000,
    TLV320_SPK_WAIT_3MS = 0b001,
    TLV320_SPK_WAIT_7MS = 0b010,
    TLV320_SPK_WAIT_12MS = 0b011,
    TLV320_SPK_WAIT_15MS = 0b100,
    TLV320_SPK_WAIT_19MS = 0b101,
    TLV320_SPK_WAIT_24MS = 0b110,
    TLV320_SPK_WAIT_30MS = 0b111,
} tlv320_spk_wait_t;

typedef enum {
    TLV320_DAC_ROUTE_NONE = 0b00,
    TLV320_DAC_ROUTE_MIXER = 0b01,
    TLV320_DAC_ROUTE_HP = 0b10,
} tlv320_dac_route_t;

typedef enum {
    TLV320_MICBIAS_OFF = 0b00,
    TLV320_MICBIAS_2V = 0b01,
    TLV320_MICBIAS_2_5V = 0b10,
    TLV320_MICBIAS_AVDD = 0b11,
} tlv320_micbias_volt_t;

typedef enum {
    TLV320_SPK_GAIN_6DB = 0b00,
    TLV320_SPK_GAIN_12DB = 0b01,
    TLV320_SPK_GAIN_18DB = 0b10,
    TLV320_SPK_GAIN_24DB = 0b11,
} tlv320_spk_gain_t;

typedef enum {
    TLV320DAC3100_BCLK_SRC_DAC_CLK = 0,
    TLV320DAC3100_BCLK_SRC_DAC_MOD_CLK = 1,
} tlv320dac3100_bclk_src_t;

// Device structure
typedef struct {
    i2c_inst_t *i2c;
    uint8_t addr;
    uint8_t current_page;
} tlv320dac3100_t;

// Core functions
bool tlv320_init(tlv320dac3100_t *dev, i2c_inst_t *i2c, uint8_t addr);
bool tlv320_reset(tlv320dac3100_t *dev);
bool tlv320_is_overtemperature(tlv320dac3100_t *dev);

// Clock and PLL functions
bool tlv320_set_pll_clock_input(tlv320dac3100_t *dev, tlv320dac3100_pll_clkin_t clkin);
tlv320dac3100_pll_clkin_t tlv320_get_pll_clock_input(tlv320dac3100_t *dev);
bool tlv320_set_codec_clock_input(tlv320dac3100_t *dev, tlv320dac3100_codec_clkin_t clkin);
tlv320dac3100_codec_clkin_t tlv320_get_codec_clock_input(tlv320dac3100_t *dev);
bool tlv320_set_clock_divider_input(tlv320dac3100_t *dev, tlv320dac3100_cdiv_clkin_t clkin);
tlv320dac3100_cdiv_clkin_t tlv320_get_clock_divider_input(tlv320dac3100_t *dev);

bool tlv320_power_pll(tlv320dac3100_t *dev, bool on);
bool tlv320_is_pll_powered(tlv320dac3100_t *dev);
bool tlv320_set_pll_values(tlv320dac3100_t *dev, uint8_t P, uint8_t R, uint8_t J, uint16_t D);
bool tlv320_get_pll_values(tlv320dac3100_t *dev, uint8_t *P, uint8_t *R, uint8_t *J, uint16_t *D);
bool tlv320_configure_pll(tlv320dac3100_t *dev, uint32_t mclk_freq, uint32_t desired_freq, float max_error);
bool tlv320_validate_pll_config(uint8_t P, uint8_t R, uint8_t J, uint16_t D, float pll_clkin);

bool tlv320_set_ndac(tlv320dac3100_t *dev, bool enable, uint8_t val);
bool tlv320_get_ndac(tlv320dac3100_t *dev, bool *enabled, uint8_t *val);
bool tlv320_set_mdac(tlv320dac3100_t *dev, bool enable, uint8_t val);
bool tlv320_get_mdac(tlv320dac3100_t *dev, bool *enabled, uint8_t *val);
bool tlv320_set_dosr(tlv320dac3100_t *dev, uint16_t val);
bool tlv320_get_dosr(tlv320dac3100_t *dev, uint16_t *val);
bool tlv320_set_clkout_m(tlv320dac3100_t *dev, bool enable, uint8_t val);
bool tlv320_get_clkout_m(tlv320dac3100_t *dev, bool *enabled, uint8_t *val);
bool tlv320_set_bclk_n(tlv320dac3100_t *dev, bool enable, uint8_t val);
bool tlv320_get_bclk_n(tlv320dac3100_t *dev, bool *enabled, uint8_t *val);
bool tlv320_set_bclk_offset(tlv320dac3100_t *dev, uint8_t offset);
bool tlv320_get_bclk_offset(tlv320dac3100_t *dev, uint8_t *offset);
bool tlv320_set_bclk_config(tlv320dac3100_t *dev, bool invert_bclk, bool active_when_powered_down, 
                             tlv320dac3100_bclk_src_t source);
bool tlv320_get_bclk_config(tlv320dac3100_t *dev, bool *invert_bclk, bool *active_when_powered_down,
                             tlv320dac3100_bclk_src_t *source);

// Codec interface functions
bool tlv320_set_codec_interface(tlv320dac3100_t *dev, tlv320dac3100_format_t format,
                                 tlv320dac3100_data_len_t len, bool bclk_out, bool wclk_out);
bool tlv320_get_codec_interface(tlv320dac3100_t *dev, tlv320dac3100_format_t *format,
                                 tlv320dac3100_data_len_t *len, bool *bclk_out, bool *wclk_out);

// DAC functions
bool tlv320_set_dac_processing_block(tlv320dac3100_t *dev, uint8_t block_number);
uint8_t tlv320_get_dac_processing_block(tlv320dac3100_t *dev);
bool tlv320_set_dac_data_path(tlv320dac3100_t *dev, bool left_dac_on, bool right_dac_on,
                               tlv320_dac_path_t left_path, tlv320_dac_path_t right_path,
                               tlv320_volume_step_t volume_step);
bool tlv320_get_dac_data_path(tlv320dac3100_t *dev, bool *left_dac_on, bool *right_dac_on,
                               tlv320_dac_path_t *left_path, tlv320_dac_path_t *right_path,
                               tlv320_volume_step_t *volume_step);
bool tlv320_set_dac_volume_control(tlv320dac3100_t *dev, bool left_mute, bool right_mute,
                                    tlv320_vol_control_t control);
bool tlv320_get_dac_volume_control(tlv320dac3100_t *dev, bool *left_mute, bool *right_mute,
                                    tlv320_vol_control_t *control);
bool tlv320_set_channel_volume(tlv320dac3100_t *dev, bool right_channel, float dB);
float tlv320_get_channel_volume(tlv320dac3100_t *dev, bool right_channel);
bool tlv320_get_dac_flags(tlv320dac3100_t *dev, bool *left_dac_powered, bool *hpl_powered,
                           bool *left_classd_powered, bool *right_dac_powered,
                           bool *hpr_powered, bool *right_classd_powered,
                           bool *left_pga_gain_ok, bool *right_pga_gain_ok);

// Headphone and speaker functions
bool tlv320_configure_headphone_driver(tlv320dac3100_t *dev, bool left_powered, bool right_powered,
                                        tlv320_hp_common_t common, bool powerDownOnSCD);
bool tlv320_configure_headphone_pop(tlv320dac3100_t *dev, bool wait_for_powerdown,
                                     tlv320_hp_time_t powerup_time, tlv320_ramp_time_t ramp_time);
bool tlv320_configure_analog_inputs(tlv320dac3100_t *dev, tlv320_dac_route_t left_dac,
                                     tlv320_dac_route_t right_dac, bool left_ain1,
                                     bool left_ain2, bool right_ain2, bool hpl_routed_to_hpr);
bool tlv320_set_hpl_volume(tlv320dac3100_t *dev, bool route_enabled, uint8_t gain);
bool tlv320_set_hpr_volume(tlv320dac3100_t *dev, bool route_enabled, uint8_t gain);
bool tlv320_set_spk_volume(tlv320dac3100_t *dev, bool route_enabled, uint8_t gain);
bool tlv320_configure_hpl_pga(tlv320dac3100_t *dev, uint8_t gain_db, bool unmute);
bool tlv320_configure_hpr_pga(tlv320dac3100_t *dev, uint8_t gain_db, bool unmute);
bool tlv320_configure_spk_pga(tlv320dac3100_t *dev, tlv320_spk_gain_t gain, bool unmute);
bool tlv320_is_hpl_gain_applied(tlv320dac3100_t *dev);
bool tlv320_is_hpr_gain_applied(tlv320dac3100_t *dev);
bool tlv320_is_spk_gain_applied(tlv320dac3100_t *dev);
bool tlv320_is_headphone_shorted(tlv320dac3100_t *dev);
bool tlv320_enable_speaker(tlv320dac3100_t *dev, bool en);
bool tlv320_speaker_enabled(tlv320dac3100_t *dev);
bool tlv320_is_speaker_shorted(tlv320dac3100_t *dev);
bool tlv320_headphone_lineout(tlv320dac3100_t *dev, bool left, bool right);
bool tlv320_set_speaker_wait_time(tlv320dac3100_t *dev, tlv320_spk_wait_t wait_time);
bool tlv320_reset_speaker_on_scd(tlv320dac3100_t *dev, bool reset);
bool tlv320_reset_headphone_on_scd(tlv320dac3100_t *dev, bool reset);

// Headset detection
bool tlv320_set_headset_detect(tlv320dac3100_t *dev, bool enable,
                                tlv320_detect_debounce_t detect_debounce,
                                tlv320_button_debounce_t button_debounce);
tlv320_headset_status_t tlv320_get_headset_status(tlv320dac3100_t *dev);

// Beep generator
bool tlv320_is_beeping(tlv320dac3100_t *dev);
bool tlv320_enable_beep(tlv320dac3100_t *dev, bool enable);
bool tlv320_set_beep_volume(tlv320dac3100_t *dev, int8_t left_dB, int8_t right_dB);
bool tlv320_set_beep_length(tlv320dac3100_t *dev, uint32_t samples);
bool tlv320_set_beep_sincos(tlv320dac3100_t *dev, uint16_t sin_val, uint16_t cos_val);
bool tlv320_configure_beep_tone(tlv320dac3100_t *dev, float frequency, uint32_t duration_ms,
                                 uint32_t sample_rate);

// GPIO and interrupt functions
bool tlv320_set_gpio1_mode(tlv320dac3100_t *dev, tlv320_gpio1_mode_t mode);
tlv320_gpio1_mode_t tlv320_get_gpio1_mode(tlv320dac3100_t *dev);
bool tlv320_set_gpio1_output(tlv320dac3100_t *dev, bool value);
bool tlv320_get_gpio1_input(tlv320dac3100_t *dev);
bool tlv320_set_din_mode(tlv320dac3100_t *dev, tlv320_din_mode_t mode);
tlv320_din_mode_t tlv320_get_din_mode(tlv320dac3100_t *dev);
bool tlv320_get_din_input(tlv320dac3100_t *dev);
bool tlv320_set_int1_source(tlv320dac3100_t *dev, bool headset_detect, bool button_press,
                             bool dac_drc, bool agc_noise, bool over_current, bool multiple_pulse);
bool tlv320_set_int2_source(tlv320dac3100_t *dev, bool headset_detect, bool button_press,
                             bool dac_drc, bool agc_noise, bool over_current, bool multiple_pulse);
uint8_t tlv320_read_irq_flags(tlv320dac3100_t *dev, bool sticky);

// Miscellaneous functions
bool tlv320_config_vol_adc(tlv320dac3100_t *dev, bool pin_control, bool use_mclk,
                            tlv320_vol_hyst_t hysteresis, tlv320_vol_rate_t rate);
float tlv320_read_vol_adc_db(tlv320dac3100_t *dev);
bool tlv320_config_micbias(tlv320dac3100_t *dev, bool power_down, bool always_on,
                            tlv320_micbias_volt_t voltage);
bool tlv320_set_input_common_mode(tlv320dac3100_t *dev, bool ain1_cm, bool ain2_cm);
bool tlv320_config_delay_divider(tlv320dac3100_t *dev, bool use_mclk, uint8_t divider);

// Low-level register access
uint8_t tlv320_read_register(tlv320dac3100_t *dev, uint8_t page, uint8_t reg);
bool tlv320_write_register(tlv320dac3100_t *dev, uint8_t page, uint8_t reg, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif // _PICO_TLV320DAC3100_H
