/**
 * @file pico_tlv320dac3100.c
 * @brief Raspberry Pi Pico SDK implementation for TI TLV320DAC3100
 *
 * This is a port of the Adafruit TLV320DAC3100 library to native Pico SDK.
 * All Arduino and Adafruit_BusIO dependencies have been replaced with native
 * Pico SDK I2C functions.
 *
 * MIT License - see LICENSE.txt for details
 */

#include "pico_tlv320dac3100.h"
#include "pico/stdlib.h"
#include <math.h>
#include <string.h>

// Helper macros for bit manipulation
#define BIT_MASK(width) ((1u << (width)) - 1)
#define GET_BITS(val, shift, width) (((val) >> (shift)) & BIT_MASK(width))
#define SET_BITS(val, bits, shift, width) (((val) & ~(BIT_MASK(width) << (shift))) | (((bits) & BIT_MASK(width)) << (shift)))

// Internal helper functions
static bool set_page(tlv320dac3100_t *dev, uint8_t page) {
    if (dev->current_page == page) {
        return true;
    }
    
    uint8_t data[2] = {TLV320DAC3100_REG_PAGE_SELECT, page};
    int result = i2c_write_blocking(dev->i2c, dev->addr, data, 2, false);
    
    if (result == 2) {
        dev->current_page = page;
        return true;
    }
    return false;
}

static uint8_t get_page(tlv320dac3100_t *dev) {
    return dev->current_page;
}

static bool write_register(tlv320dac3100_t *dev, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    int result = i2c_write_blocking(dev->i2c, dev->addr, data, 2, false);
    return (result == 2);
}

static bool read_register(tlv320dac3100_t *dev, uint8_t reg, uint8_t *value) {
    int result = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, true);
    if (result != 1) return false;
    
    result = i2c_read_blocking(dev->i2c, dev->addr, value, 1, false);
    return (result == 1);
}

static bool modify_register(tlv320dac3100_t *dev, uint8_t reg, uint8_t mask, uint8_t value) {
    uint8_t current;
    if (!read_register(dev, reg, &current)) {
        return false;
    }
    uint8_t new_val = (current & ~mask) | (value & mask);
    return write_register(dev, reg, new_val);
}

static bool read_bits(tlv320dac3100_t *dev, uint8_t reg, uint8_t shift, uint8_t width, uint8_t *value) {
    uint8_t reg_val;
    if (!read_register(dev, reg, &reg_val)) {
        return false;
    }
    *value = GET_BITS(reg_val, shift, width);
    return true;
}

static bool write_bits(tlv320dac3100_t *dev, uint8_t reg, uint8_t shift, uint8_t width, uint8_t value) {
    uint8_t reg_val;
    if (!read_register(dev, reg, &reg_val)) {
        return false;
    }
    uint8_t new_val = SET_BITS(reg_val, value, shift, width);
    return write_register(dev, reg, new_val);
}

// Core API functions
bool tlv320_init(tlv320dac3100_t *dev, i2c_inst_t *i2c, uint8_t addr) {
    if (!dev || !i2c) return false;
    
    dev->i2c = i2c;
    dev->addr = addr;
    dev->current_page = 0xFF; // Force page select on first access
    
    return tlv320_reset(dev);
}

bool tlv320_reset(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_RESET, 0, 1, 1)) return false;
    
    sleep_ms(10);
    
    uint8_t reset_bit;
    if (!read_bits(dev, TLV320DAC3100_REG_RESET, 0, 1, &reset_bit)) return false;
    
    dev->current_page = 0xFF; // Force page reselect after reset
    return (reset_bit == 0);
}

bool tlv320_is_overtemperature(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return false;
    
    uint8_t ot_flag;
    if (!read_bits(dev, TLV320DAC3100_REG_OT_FLAG, 1, 1, &ot_flag)) return false;
    
    return !ot_flag; // Inverted: 0 = overtemp
}

// Clock and PLL functions
bool tlv320_set_pll_clock_input(tlv320dac3100_t *dev, tlv320dac3100_pll_clkin_t clkin) {
    if (!set_page(dev, 0)) return false;
    return write_bits(dev, TLV320DAC3100_REG_CLOCK_MUX1, 2, 2, clkin);
}

tlv320dac3100_pll_clkin_t tlv320_get_pll_clock_input(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return TLV320DAC3100_PLL_CLKIN_MCLK;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_CLOCK_MUX1, 2, 2, &val)) {
        return TLV320DAC3100_PLL_CLKIN_MCLK;
    }
    return (tlv320dac3100_pll_clkin_t)val;
}

bool tlv320_set_codec_clock_input(tlv320dac3100_t *dev, tlv320dac3100_codec_clkin_t clkin) {
    if (!set_page(dev, 0)) return false;
    return write_bits(dev, TLV320DAC3100_REG_CLOCK_MUX1, 0, 2, clkin);
}

tlv320dac3100_codec_clkin_t tlv320_get_codec_clock_input(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return TLV320DAC3100_CODEC_CLKIN_MCLK;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_CLOCK_MUX1, 0, 2, &val)) {
        return TLV320DAC3100_CODEC_CLKIN_MCLK;
    }
    return (tlv320dac3100_codec_clkin_t)val;
}

bool tlv320_set_clock_divider_input(tlv320dac3100_t *dev, tlv320dac3100_cdiv_clkin_t clkin) {
    if (!set_page(dev, 0)) return false;
    return write_bits(dev, TLV320DAC3100_REG_CLKOUT_MUX, 0, 3, clkin);
}

tlv320dac3100_cdiv_clkin_t tlv320_get_clock_divider_input(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return TLV320DAC3100_CDIV_CLKIN_MCLK;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_CLKOUT_MUX, 3, 3, &val)) {
        return TLV320DAC3100_CDIV_CLKIN_MCLK;
    }
    return (tlv320dac3100_cdiv_clkin_t)val;
}

bool tlv320_power_pll(tlv320dac3100_t *dev, bool on) {
    if (!set_page(dev, 0)) return false;
    return write_bits(dev, TLV320DAC3100_REG_PLL_PROG_PR, 7, 1, on ? 1 : 0);
}

bool tlv320_is_pll_powered(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return false;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_PLL_PROG_PR, 7, 1, &val)) return false;
    return (val != 0);
}

bool tlv320_set_pll_values(tlv320dac3100_t *dev, uint8_t P, uint8_t R, uint8_t J, uint16_t D) {
    if (!set_page(dev, 0)) return false;
    
    // Validate ranges
    if (P < 1 || P > 8 || R < 1 || R > 16 || J < 1 || J > 63 || D > 9999) {
        return false;
    }
    
    // Write P & R
    if (!write_bits(dev, TLV320DAC3100_REG_PLL_PROG_PR, 4, 3, P % 8)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_PLL_PROG_PR, 0, 4, R % 16)) return false;
    
    // Write J
    if (!write_bits(dev, TLV320DAC3100_REG_PLL_PROG_J, 0, 6, J)) return false;
    
    // Write D (14 bits split across two registers)
    if (!write_register(dev, TLV320DAC3100_REG_PLL_PROG_D_MSB, D >> 8)) return false;
    if (!write_register(dev, TLV320DAC3100_REG_PLL_PROG_D_LSB, D & 0xFF)) return false;
    
    return true;
}

bool tlv320_get_pll_values(tlv320dac3100_t *dev, uint8_t *P, uint8_t *R, uint8_t *J, uint16_t *D) {
    if (!set_page(dev, 0)) return false;
    
    if (P) {
        uint8_t p_val;
        if (!read_bits(dev, TLV320DAC3100_REG_PLL_PROG_PR, 4, 3, &p_val)) return false;
        *P = (p_val == 0) ? 8 : p_val;
    }
    
    if (R) {
        uint8_t r_val;
        if (!read_bits(dev, TLV320DAC3100_REG_PLL_PROG_PR, 0, 4, &r_val)) return false;
        *R = (r_val == 0) ? 16 : r_val;
    }
    
    if (J) {
        if (!read_bits(dev, TLV320DAC3100_REG_PLL_PROG_J, 0, 6, J)) return false;
    }
    
    if (D) {
        uint8_t msb, lsb;
        if (!read_register(dev, TLV320DAC3100_REG_PLL_PROG_D_MSB, &msb)) return false;
        if (!read_register(dev, TLV320DAC3100_REG_PLL_PROG_D_LSB, &lsb)) return false;
        *D = ((uint16_t)msb << 8) | lsb;
    }
    
    return true;
}

bool tlv320_configure_pll(tlv320dac3100_t *dev, uint32_t mclk_freq, uint32_t desired_freq, float max_error) {
    float ratio = (float)desired_freq / mclk_freq;
    float best_error = 1.0;
    uint8_t best_P = 1, best_R = 1, best_J = 0;
    uint16_t best_D = 0;
    
    for (uint8_t P = 1; P <= 8; P++) {
        for (uint8_t R = 1; R <= 16; R++) {
            float J_float = ratio * P * R;
            if (J_float > 63) continue;
            
            uint8_t J = (uint8_t)J_float;
            uint16_t D = (uint16_t)((J_float - J) * 2048);
            if (D > 2047) continue;
            
            float actual_ratio = (float)(J + (float)D / 2048.0f) / (P * R);
            float error = fabsf(actual_ratio - ratio) / ratio;
            
            if (error < best_error) {
                best_error = error;
                best_P = P;
                best_R = R;
                best_J = J;
                best_D = D;
                
                if (error <= max_error) {
                    return tlv320_set_pll_values(dev, P, R, J, D);
                }
            }
        }
    }
    
    if (best_error < 0.1f) {
        return tlv320_set_pll_values(dev, best_P, best_R, best_J, best_D);
    }
    
    return false;
}

bool tlv320_validate_pll_config(uint8_t P, uint8_t R, uint8_t J, uint16_t D, float pll_clkin) {
    float pll_in_div_p = pll_clkin / P;
    float jd = J + (float)D / 2048.0f;
    float pll_out = (pll_clkin * jd * R) / P;
    
    if (D == 0) {
        if (pll_in_div_p < 512000 || pll_in_div_p > 20000000) return false;
        if (pll_out < 80000000 || pll_out > 110000000) return false;
        if ((R * J) < 4 || (R * J) > 259) return false;
    } else {
        if (pll_in_div_p < 10000000 || pll_in_div_p > 20000000) return false;
        if (pll_out < 80000000 || pll_out > 110000000) return false;
        if (R != 1) return false;
    }
    return true;
}

bool tlv320_set_ndac(tlv320dac3100_t *dev, bool enable, uint8_t val) {
    if (!set_page(dev, 0)) return false;
    if (val < 1 || val > 128) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_NDAC, 7, 1, enable ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_NDAC, 0, 7, val % 128)) return false;
    
    return true;
}

bool tlv320_get_ndac(tlv320dac3100_t *dev, bool *enabled, uint8_t *val) {
    if (!set_page(dev, 0)) return false;
    
    if (enabled) {
        uint8_t en;
        if (!read_bits(dev, TLV320DAC3100_REG_NDAC, 7, 1, &en)) return false;
        *enabled = (en != 0);
    }
    
    if (val) {
        uint8_t v;
        if (!read_bits(dev, TLV320DAC3100_REG_NDAC, 0, 7, &v)) return false;
        *val = (v == 0) ? 128 : v;
    }
    
    return true;
}

bool tlv320_set_mdac(tlv320dac3100_t *dev, bool enable, uint8_t val) {
    if (!set_page(dev, 0)) return false;
    if (val < 1 || val > 128) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_MDAC, 7, 1, enable ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_MDAC, 0, 7, val % 128)) return false;
    
    return true;
}

bool tlv320_get_mdac(tlv320dac3100_t *dev, bool *enabled, uint8_t *val) {
    if (!set_page(dev, 0)) return false;
    
    if (enabled) {
        uint8_t en;
        if (!read_bits(dev, TLV320DAC3100_REG_MDAC, 7, 1, &en)) return false;
        *enabled = (en != 0);
    }
    
    if (val) {
        uint8_t v;
        if (!read_bits(dev, TLV320DAC3100_REG_MDAC, 0, 7, &v)) return false;
        *val = (v == 0) ? 128 : v;
    }
    
    return true;
}

bool tlv320_set_dosr(tlv320dac3100_t *dev, uint16_t val) {
    if (!set_page(dev, 0)) return false;
    if (val < 2 || val > 1024 || val == 1023) return false;
    
    uint16_t dosr_val = val % 1024;
    
    if (!write_register(dev, TLV320DAC3100_REG_DOSR_MSB, dosr_val >> 8)) return false;
    if (!write_register(dev, TLV320DAC3100_REG_DOSR_LSB, dosr_val & 0xFF)) return false;
    
    return true;
}

bool tlv320_get_dosr(tlv320dac3100_t *dev, uint16_t *val) {
    if (!set_page(dev, 0) || !val) return false;
    
    uint8_t msb, lsb;
    if (!read_register(dev, TLV320DAC3100_REG_DOSR_MSB, &msb)) return false;
    if (!read_register(dev, TLV320DAC3100_REG_DOSR_LSB, &lsb)) return false;
    
    uint16_t v = ((uint16_t)msb << 8) | lsb;
    *val = (v == 0) ? 1024 : v;
    
    return true;
}

bool tlv320_set_clkout_m(tlv320dac3100_t *dev, bool enable, uint8_t val) {
    if (!set_page(dev, 0)) return false;
    if (val < 1 || val > 128) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_CLKOUT_M, 7, 1, enable ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_CLKOUT_M, 0, 7, val % 128)) return false;
    
    return true;
}

bool tlv320_get_clkout_m(tlv320dac3100_t *dev, bool *enabled, uint8_t *val) {
    if (!set_page(dev, 0)) return false;
    
    if (enabled) {
        uint8_t en;
        if (!read_bits(dev, TLV320DAC3100_REG_CLKOUT_M, 7, 1, &en)) return false;
        *enabled = (en != 0);
    }
    
    if (val) {
        uint8_t v;
        if (!read_bits(dev, TLV320DAC3100_REG_CLKOUT_M, 0, 7, &v)) return false;
        *val = (v == 0) ? 128 : v;
    }
    
    return true;
}

bool tlv320_set_bclk_offset(tlv320dac3100_t *dev, uint8_t offset) {
    if (!set_page(dev, 0)) return false;
    return write_register(dev, TLV320DAC3100_REG_DATA_SLOT_OFFSET, offset);
}

bool tlv320_get_bclk_offset(tlv320dac3100_t *dev, uint8_t *offset) {
    if (!set_page(dev, 0) || !offset) return false;
    return read_register(dev, TLV320DAC3100_REG_DATA_SLOT_OFFSET, offset);
}

bool tlv320_set_bclk_n(tlv320dac3100_t *dev, bool enable, uint8_t val) {
    if (!set_page(dev, 0)) return false;
    if (val < 1 || val > 128) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_BCLK_N, 7, 1, enable ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_BCLK_N, 0, 7, val % 128)) return false;
    
    return true;
}

bool tlv320_get_bclk_n(tlv320dac3100_t *dev, bool *enabled, uint8_t *val) {
    if (!set_page(dev, 0)) return false;
    
    if (enabled) {
        uint8_t en;
        if (!read_bits(dev, TLV320DAC3100_REG_BCLK_N, 7, 1, &en)) return false;
        *enabled = (en != 0);
    }
    
    if (val) {
        uint8_t v;
        if (!read_bits(dev, TLV320DAC3100_REG_BCLK_N, 0, 7, &v)) return false;
        *val = (v == 0) ? 128 : v;
    }
    
    return true;
}

bool tlv320_set_bclk_config(tlv320dac3100_t *dev, bool invert_bclk, bool active_when_powered_down,
                             tlv320dac3100_bclk_src_t source) {
    if (!set_page(dev, 0)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_BCLK_CTRL2, 3, 1, invert_bclk ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_BCLK_CTRL2, 2, 1, active_when_powered_down ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_BCLK_CTRL2, 0, 2, source)) return false;
    
    return true;
}

bool tlv320_get_bclk_config(tlv320dac3100_t *dev, bool *invert_bclk, bool *active_when_powered_down,
                             tlv320dac3100_bclk_src_t *source) {
    if (!set_page(dev, 0)) return false;
    
    if (invert_bclk) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_BCLK_CTRL2, 3, 1, &val)) return false;
        *invert_bclk = (val != 0);
    }
    
    if (active_when_powered_down) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_BCLK_CTRL2, 2, 1, &val)) return false;
        *active_when_powered_down = (val != 0);
    }
    
    if (source) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_BCLK_CTRL2, 0, 2, &val)) return false;
        *source = (tlv320dac3100_bclk_src_t)val;
    }
    
    return true;
}

// Codec interface functions
bool tlv320_set_codec_interface(tlv320dac3100_t *dev, tlv320dac3100_format_t format,
                                 tlv320dac3100_data_len_t len, bool bclk_out, bool wclk_out) {
    if (!set_page(dev, 0)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_CODEC_IF_CTRL1, 6, 2, format)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_CODEC_IF_CTRL1, 4, 2, len)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_CODEC_IF_CTRL1, 3, 1, bclk_out ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_CODEC_IF_CTRL1, 2, 1, wclk_out ? 1 : 0)) return false;
    
    return true;
}

bool tlv320_get_codec_interface(tlv320dac3100_t *dev, tlv320dac3100_format_t *format,
                                 tlv320dac3100_data_len_t *len, bool *bclk_out, bool *wclk_out) {
    if (!set_page(dev, 0)) return false;
    
    if (format) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_CODEC_IF_CTRL1, 6, 2, &val)) return false;
        *format = (tlv320dac3100_format_t)val;
    }
    
    if (len) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_CODEC_IF_CTRL1, 4, 2, &val)) return false;
        *len = (tlv320dac3100_data_len_t)val;
    }
    
    if (bclk_out) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_CODEC_IF_CTRL1, 3, 1, &val)) return false;
        *bclk_out = (val != 0);
    }
    
    if (wclk_out) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_CODEC_IF_CTRL1, 2, 1, &val)) return false;
        *wclk_out = (val != 0);
    }
    
    return true;
}

// DAC functions
bool tlv320_set_dac_processing_block(tlv320dac3100_t *dev, uint8_t block_number) {
    if (block_number < 1 || block_number > 25) return false;
    if (!set_page(dev, 0)) return false;
    
    return write_bits(dev, TLV320DAC3100_REG_DAC_PRB, 0, 5, block_number);
}

uint8_t tlv320_get_dac_processing_block(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return 0;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_DAC_PRB, 0, 5, &val)) return 0;
    return val;
}

bool tlv320_set_dac_data_path(tlv320dac3100_t *dev, bool left_dac_on, bool right_dac_on,
                               tlv320_dac_path_t left_path, tlv320_dac_path_t right_path,
                               tlv320_volume_step_t volume_step) {
    if (!set_page(dev, 0)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 7, 1, left_dac_on ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 6, 1, right_dac_on ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 4, 2, left_path)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 2, 2, right_path)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 0, 2, volume_step)) return false;
    
    return true;
}

bool tlv320_get_dac_data_path(tlv320dac3100_t *dev, bool *left_dac_on, bool *right_dac_on,
                               tlv320_dac_path_t *left_path, tlv320_dac_path_t *right_path,
                               tlv320_volume_step_t *volume_step) {
    if (!set_page(dev, 0)) return false;
    
    if (left_dac_on) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 7, 1, &val)) return false;
        *left_dac_on = (val != 0);
    }
    
    if (right_dac_on) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 6, 1, &val)) return false;
        *right_dac_on = (val != 0);
    }
    
    if (left_path) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 4, 2, &val)) return false;
        *left_path = (tlv320_dac_path_t)val;
    }
    
    if (right_path) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 2, 2, &val)) return false;
        *right_path = (tlv320_dac_path_t)val;
    }
    
    if (volume_step) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_DATAPATH, 0, 2, &val)) return false;
        *volume_step = (tlv320_volume_step_t)val;
    }
    
    return true;
}

bool tlv320_set_dac_volume_control(tlv320dac3100_t *dev, bool left_mute, bool right_mute,
                                    tlv320_vol_control_t control) {
    if (!set_page(dev, 0)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_DAC_VOL_CTRL, 3, 1, left_mute ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_DAC_VOL_CTRL, 2, 1, right_mute ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_DAC_VOL_CTRL, 0, 2, control)) return false;
    
    return true;
}

bool tlv320_get_dac_volume_control(tlv320dac3100_t *dev, bool *left_mute, bool *right_mute,
                                    tlv320_vol_control_t *control) {
    if (!set_page(dev, 0)) return false;
    
    if (left_mute) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_VOL_CTRL, 3, 1, &val)) return false;
        *left_mute = (val != 0);
    }
    
    if (right_mute) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_VOL_CTRL, 2, 1, &val)) return false;
        *right_mute = (val != 0);
    }
    
    if (control) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_VOL_CTRL, 0, 2, &val)) return false;
        *control = (tlv320_vol_control_t)val;
    }
    
    return true;
}

bool tlv320_set_channel_volume(tlv320dac3100_t *dev, bool right_channel, float dB) {
    if (!set_page(dev, 0)) return false;
    
    // Constrain to valid range
    if (dB > 24.0f) dB = 24.0f;
    if (dB < -63.5f) dB = -63.5f;
    
    int8_t reg_val = (int8_t)(dB * 2);
    
    // Check for reserved values
    if ((reg_val == 0x80) || (reg_val > 0x30)) return false;
    
    uint8_t reg = right_channel ? TLV320DAC3100_REG_DAC_RVOL : TLV320DAC3100_REG_DAC_LVOL;
    return write_register(dev, reg, (uint8_t)reg_val);
}

float tlv320_get_channel_volume(tlv320dac3100_t *dev, bool right_channel) {
    if (!set_page(dev, 0)) return 0.0f;
    
    uint8_t reg = right_channel ? TLV320DAC3100_REG_DAC_RVOL : TLV320DAC3100_REG_DAC_LVOL;
    uint8_t reg_val;
    
    if (!read_register(dev, reg, &reg_val)) return 0.0f;
    
    // Convert from two's complement
    int8_t steps;
    if (reg_val & 0x80) {
        steps = (int8_t)(reg_val - 256);
    } else {
        steps = (int8_t)reg_val;
    }
    
    return steps * 0.5f;
}

bool tlv320_get_dac_flags(tlv320dac3100_t *dev, bool *left_dac_powered, bool *hpl_powered,
                           bool *left_classd_powered, bool *right_dac_powered,
                           bool *hpr_powered, bool *right_classd_powered,
                           bool *left_pga_gain_ok, bool *right_pga_gain_ok) {
    if (!set_page(dev, 0)) return false;
    
    if (left_dac_powered) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_FLAG, 7, 1, &val)) return false;
        *left_dac_powered = (val != 0);
    }
    
    if (hpl_powered) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_FLAG, 5, 1, &val)) return false;
        *hpl_powered = (val != 0);
    }
    
    if (left_classd_powered) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_FLAG, 4, 1, &val)) return false;
        *left_classd_powered = (val != 0);
    }
    
    if (right_dac_powered) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_FLAG, 3, 1, &val)) return false;
        *right_dac_powered = (val != 0);
    }
    
    if (hpr_powered) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_FLAG, 1, 1, &val)) return false;
        *hpr_powered = (val != 0);
    }
    
    if (right_classd_powered) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_FLAG, 0, 1, &val)) return false;
        *right_classd_powered = (val != 0);
    }
    
    if (left_pga_gain_ok) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_FLAG2, 4, 1, &val)) return false;
        *left_pga_gain_ok = (val != 0);
    }
    
    if (right_pga_gain_ok) {
        uint8_t val;
        if (!read_bits(dev, TLV320DAC3100_REG_DAC_FLAG2, 0, 1, &val)) return false;
        *right_pga_gain_ok = (val != 0);
    }
    
    return true;
}

// Continue in next part due to length...
// Headphone and speaker functions
bool tlv320_configure_headphone_driver(tlv320dac3100_t *dev, bool left_powered, bool right_powered,
                                        tlv320_hp_common_t common, bool powerDownOnSCD) {
    if (!set_page(dev, 1)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_HP_DRIVERS, 7, 1, left_powered ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HP_DRIVERS, 6, 1, right_powered ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HP_DRIVERS, 3, 2, common)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HP_DRIVERS, 1, 1, powerDownOnSCD ? 1 : 0)) return false;
    
    return true;
}

bool tlv320_configure_headphone_pop(tlv320dac3100_t *dev, bool wait_for_powerdown,
                                     tlv320_hp_time_t powerup_time, tlv320_ramp_time_t ramp_time) {
    if (!set_page(dev, 1)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_HP_POP, 7, 1, wait_for_powerdown ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HP_POP, 3, 4, powerup_time)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HP_POP, 1, 2, ramp_time)) return false;
    
    return true;
}

bool tlv320_configure_analog_inputs(tlv320dac3100_t *dev, tlv320_dac_route_t left_dac,
                                     tlv320_dac_route_t right_dac, bool left_ain1,
                                     bool left_ain2, bool right_ain2, bool hpl_routed_to_hpr) {
    if (!set_page(dev, 1)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_OUT_ROUTING, 6, 2, left_dac)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_OUT_ROUTING, 5, 1, left_ain1 ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_OUT_ROUTING, 4, 1, left_ain2 ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_OUT_ROUTING, 2, 2, right_dac)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_OUT_ROUTING, 1, 1, right_ain2 ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_OUT_ROUTING, 0, 1, hpl_routed_to_hpr ? 1 : 0)) return false;
    
    return true;
}

bool tlv320_set_hpl_volume(tlv320dac3100_t *dev, bool route_enabled, uint8_t gain) {
    if (!set_page(dev, 1)) return false;
    if (gain > 0x7F) gain = 0x7F;
    
    if (!write_bits(dev, TLV320DAC3100_REG_HPL_VOL, 7, 1, route_enabled ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HPL_VOL, 0, 7, gain)) return false;
    
    return true;
}

bool tlv320_set_hpr_volume(tlv320dac3100_t *dev, bool route_enabled, uint8_t gain) {
    if (!set_page(dev, 1)) return false;
    if (gain > 0x7F) gain = 0x7F;
    
    if (!write_bits(dev, TLV320DAC3100_REG_HPR_VOL, 7, 1, route_enabled ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HPR_VOL, 0, 7, gain)) return false;
    
    return true;
}

bool tlv320_set_spk_volume(tlv320dac3100_t *dev, bool route_enabled, uint8_t gain) {
    if (!set_page(dev, 1)) return false;
    if (gain > 0x7F) gain = 0x7F;
    
    if (!write_bits(dev, TLV320DAC3100_REG_SPK_VOL, 7, 1, route_enabled ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_SPK_VOL, 0, 7, gain)) return false;
    
    return true;
}

bool tlv320_configure_hpl_pga(tlv320dac3100_t *dev, uint8_t gain_db, bool unmute) {
    if (!set_page(dev, 1)) return false;
    if (gain_db > 9) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_HPL_DRIVER, 3, 4, gain_db)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HPL_DRIVER, 2, 1, unmute ? 1 : 0)) return false;
    
    return true;
}

bool tlv320_configure_hpr_pga(tlv320dac3100_t *dev, uint8_t gain_db, bool unmute) {
    if (!set_page(dev, 1)) return false;
    if (gain_db > 9) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_HPR_DRIVER, 3, 4, gain_db)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HPR_DRIVER, 2, 1, unmute ? 1 : 0)) return false;
    
    return true;
}

bool tlv320_configure_spk_pga(tlv320dac3100_t *dev, tlv320_spk_gain_t gain, bool unmute) {
    if (!set_page(dev, 1)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_SPK_DRIVER, 3, 2, gain)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_SPK_DRIVER, 2, 1, unmute ? 1 : 0)) return false;
    
    return true;
}

bool tlv320_is_hpl_gain_applied(tlv320dac3100_t *dev) {
    if (!set_page(dev, 1)) return false;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_HPL_DRIVER, 0, 1, &val)) return false;
    return (val != 0);
}

bool tlv320_is_hpr_gain_applied(tlv320dac3100_t *dev) {
    if (!set_page(dev, 1)) return false;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_HPR_DRIVER, 0, 1, &val)) return false;
    return (val != 0);
}

bool tlv320_is_spk_gain_applied(tlv320dac3100_t *dev) {
    if (!set_page(dev, 1)) return false;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_SPK_DRIVER, 0, 1, &val)) return false;
    return (val != 0);
}

bool tlv320_is_headphone_shorted(tlv320dac3100_t *dev) {
    if (!set_page(dev, 1)) return false;
    
    uint8_t flags = tlv320_read_irq_flags(dev, false);
    return (flags & (TLV320DAC3100_IRQ_HPL_SHORT | TLV320DAC3100_IRQ_HPR_SHORT)) != 0;
}

bool tlv320_enable_speaker(tlv320dac3100_t *dev, bool en) {
    if (!set_page(dev, 1)) return false;
    return write_bits(dev, TLV320DAC3100_REG_SPK_AMP, 7, 1, en ? 1 : 0);
}

bool tlv320_speaker_enabled(tlv320dac3100_t *dev) {
    if (!set_page(dev, 1)) return false;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_SPK_AMP, 7, 1, &val)) return false;
    return (val != 0);
}

bool tlv320_is_speaker_shorted(tlv320dac3100_t *dev) {
    if (!set_page(dev, 1)) return false;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_SPK_AMP, 0, 1, &val)) return false;
    return (val != 0);
}

bool tlv320_headphone_lineout(tlv320dac3100_t *dev, bool left, bool right) {
    if (!set_page(dev, 1)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_HP_DRIVER_CTRL, 2, 1, left ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HP_DRIVER_CTRL, 1, 1, right ? 1 : 0)) return false;
    
    return true;
}

bool tlv320_set_speaker_wait_time(tlv320dac3100_t *dev, tlv320_spk_wait_t wait_time) {
    if (!set_page(dev, 1)) return false;
    return write_bits(dev, TLV320DAC3100_REG_PGA_RAMP, 4, 3, wait_time);
}

bool tlv320_reset_speaker_on_scd(tlv320dac3100_t *dev, bool reset) {
    if (!set_page(dev, 1)) return false;
    return write_bits(dev, TLV320DAC3100_REG_HP_SPK_ERR_CTL, 1, 1, reset ? 0 : 1);
}

bool tlv320_reset_headphone_on_scd(tlv320dac3100_t *dev, bool reset) {
    if (!set_page(dev, 1)) return false;
    return write_bits(dev, TLV320DAC3100_REG_HP_SPK_ERR_CTL, 0, 1, reset ? 0 : 1);
}

// Headset detection
bool tlv320_set_headset_detect(tlv320dac3100_t *dev, bool enable,
                                tlv320_detect_debounce_t detect_debounce,
                                tlv320_button_debounce_t button_debounce) {
    if (!set_page(dev, 0)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_HEADSET_DETECT, 7, 1, enable ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HEADSET_DETECT, 2, 3, detect_debounce)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_HEADSET_DETECT, 0, 2, button_debounce)) return false;
    
    return true;
}

tlv320_headset_status_t tlv320_get_headset_status(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return TLV320_HEADSET_NONE;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_HEADSET_DETECT, 5, 2, &val)) {
        return TLV320_HEADSET_NONE;
    }
    return (tlv320_headset_status_t)val;
}

// Beep generator
bool tlv320_is_beeping(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return false;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_BEEP_L, 7, 1, &val)) return false;
    return (val != 0);
}

bool tlv320_enable_beep(tlv320dac3100_t *dev, bool enable) {
    if (!set_page(dev, 0)) return false;
    return write_bits(dev, TLV320DAC3100_REG_BEEP_L, 7, 1, enable ? 1 : 0);
}

bool tlv320_set_beep_volume(tlv320dac3100_t *dev, int8_t left_dB, int8_t right_dB) {
    if (!set_page(dev, 0)) return false;
    
    // Constrain volumes
    if (left_dB > 2) left_dB = 2;
    if (left_dB < -61) left_dB = -61;
    
    uint8_t left_reg = (uint8_t)(-left_dB + 2);
    
    bool match_volumes = (right_dB == -100);
    if (match_volumes) {
        right_dB = left_dB;
    }
    if (right_dB > 2) right_dB = 2;
    if (right_dB < -61) right_dB = -61;
    uint8_t right_reg = (uint8_t)(-right_dB + 2);
    
    if (!write_bits(dev, TLV320DAC3100_REG_BEEP_L, 0, 6, left_reg)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_BEEP_R, 0, 6, right_reg)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_BEEP_R, 6, 2, match_volumes ? 0b01 : 0b00)) return false;
    
    return true;
}

bool tlv320_set_beep_length(tlv320dac3100_t *dev, uint32_t samples) {
    if (!set_page(dev, 0)) return false;
    
    samples &= 0x00FFFFFF;
    
    if (!write_register(dev, TLV320DAC3100_REG_BEEP_LEN_MSB, samples >> 16)) return false;
    if (!write_register(dev, TLV320DAC3100_REG_BEEP_LEN_MID, samples >> 8)) return false;
    if (!write_register(dev, TLV320DAC3100_REG_BEEP_LEN_LSB, samples)) return false;
    
    return true;
}

bool tlv320_set_beep_sincos(tlv320dac3100_t *dev, uint16_t sin_val, uint16_t cos_val) {
    if (!set_page(dev, 0)) return false;
    
    if (!write_register(dev, TLV320DAC3100_REG_BEEP_SIN_MSB, sin_val >> 8)) return false;
    if (!write_register(dev, TLV320DAC3100_REG_BEEP_SIN_LSB, sin_val & 0xFF)) return false;
    if (!write_register(dev, TLV320DAC3100_REG_BEEP_COS_MSB, cos_val >> 8)) return false;
    if (!write_register(dev, TLV320DAC3100_REG_BEEP_COS_LSB, cos_val & 0xFF)) return false;
    
    return true;
}

bool tlv320_configure_beep_tone(tlv320dac3100_t *dev, float frequency, uint32_t duration_ms,
                                 uint32_t sample_rate) {
    if (frequency >= (sample_rate / 4.0f)) return false;
    
    // Calculate the phase increment angle for the DDS oscillator
    // Per TLV320DAC3100 datasheet Section 7.3.8.1:
    // sin(x) and cos(x) are signed 16-bit two's complement values
    // where x = 2*pi*f/Fs (the angular frequency normalized to sample rate)
    float angle = 2.0f * M_PI * frequency / sample_rate;
    
    // Convert to signed 16-bit values (Q15 format: -1.0 to +0.99997)
    // The values are interpreted as signed two's complement in the codec
    int16_t sin_signed = (int16_t)(sinf(angle) * 32767.0f);
    int16_t cos_signed = (int16_t)(cosf(angle) * 32767.0f);
    
    // Cast to uint16_t for register write (preserves bit pattern)
    uint16_t sin_val = (uint16_t)sin_signed;
    uint16_t cos_val = (uint16_t)cos_signed;
    
    uint32_t length = (duration_ms * sample_rate) / 1000;
    if (length > 0x00FFFFFF) length = 0x00FFFFFF;
    
    // Length of 0 means infinite beep, so ensure we have a valid length
    if (length == 0) length = 1;
    
    return (tlv320_set_beep_sincos(dev, sin_val, cos_val) && tlv320_set_beep_length(dev, length));
}

// GPIO and interrupt functions
bool tlv320_set_gpio1_mode(tlv320dac3100_t *dev, tlv320_gpio1_mode_t mode) {
    if (!set_page(dev, 0)) return false;
    return write_bits(dev, TLV320DAC3100_REG_GPIO1_CTRL, 2, 4, mode);
}

tlv320_gpio1_mode_t tlv320_get_gpio1_mode(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return TLV320_GPIO1_DISABLED;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_GPIO1_CTRL, 2, 4, &val)) {
        return TLV320_GPIO1_DISABLED;
    }
    return (tlv320_gpio1_mode_t)val;
}

bool tlv320_set_gpio1_output(tlv320dac3100_t *dev, bool value) {
    if (!set_page(dev, 0)) return false;
    return write_bits(dev, TLV320DAC3100_REG_GPIO1_CTRL, 0, 1, value ? 1 : 0);
}

bool tlv320_get_gpio1_input(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return false;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_GPIO1_CTRL, 1, 1, &val)) return false;
    return (val != 0);
}

bool tlv320_set_din_mode(tlv320dac3100_t *dev, tlv320_din_mode_t mode) {
    if (!set_page(dev, 0)) return false;
    return write_bits(dev, TLV320DAC3100_REG_DIN_CTRL, 1, 2, mode);
}

tlv320_din_mode_t tlv320_get_din_mode(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return TLV320_DIN_DISABLED;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_DIN_CTRL, 1, 2, &val)) {
        return TLV320_DIN_DISABLED;
    }
    return (tlv320_din_mode_t)val;
}

bool tlv320_get_din_input(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return false;
    
    uint8_t val;
    if (!read_bits(dev, TLV320DAC3100_REG_DIN_CTRL, 0, 1, &val)) return false;
    return (val != 0);
}

bool tlv320_set_int1_source(tlv320dac3100_t *dev, bool headset_detect, bool button_press,
                             bool dac_drc, bool agc_noise, bool over_current, bool multiple_pulse) {
    if (!set_page(dev, 0)) return false;
    
    uint8_t int_config = 0;
    if (headset_detect) int_config |= (1 << 7);
    if (button_press) int_config |= (1 << 6);
    if (dac_drc) int_config |= (1 << 5);
    if (over_current) int_config |= (1 << 3);
    if (agc_noise) int_config |= (1 << 2);
    if (multiple_pulse) int_config |= (1 << 0);
    
    return write_register(dev, TLV320DAC3100_REG_INT1_CTRL, int_config);
}

bool tlv320_set_int2_source(tlv320dac3100_t *dev, bool headset_detect, bool button_press,
                             bool dac_drc, bool agc_noise, bool over_current, bool multiple_pulse) {
    if (!set_page(dev, 0)) return false;
    
    uint8_t int_config = 0;
    if (headset_detect) int_config |= (1 << 7);
    if (button_press) int_config |= (1 << 6);
    if (dac_drc) int_config |= (1 << 5);
    if (over_current) int_config |= (1 << 3);
    if (agc_noise) int_config |= (1 << 2);
    if (multiple_pulse) int_config |= (1 << 0);
    
    return write_register(dev, TLV320DAC3100_REG_INT2_CTRL, int_config);
}

uint8_t tlv320_read_irq_flags(tlv320dac3100_t *dev, bool sticky) {
    if (!set_page(dev, 0)) return 0;
    
    uint8_t reg = sticky ? TLV320DAC3100_REG_IRQ_FLAGS_STICKY : TLV320DAC3100_REG_IRQ_FLAGS;
    uint8_t flags;
    
    if (!read_register(dev, reg, &flags)) return 0;
    return flags;
}

// Miscellaneous functions
bool tlv320_config_vol_adc(tlv320dac3100_t *dev, bool pin_control, bool use_mclk,
                            tlv320_vol_hyst_t hysteresis, tlv320_vol_rate_t rate) {
    if (!set_page(dev, 0)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_VOL_ADC_CTRL, 7, 1, pin_control ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_VOL_ADC_CTRL, 6, 1, use_mclk ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_VOL_ADC_CTRL, 4, 2, hysteresis)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_VOL_ADC_CTRL, 0, 3, rate)) return false;
    
    return true;
}

float tlv320_read_vol_adc_db(tlv320dac3100_t *dev) {
    if (!set_page(dev, 0)) return 0.0f;
    
    uint8_t raw_val;
    if (!read_bits(dev, TLV320DAC3100_REG_VOL_ADC_READ, 0, 7, &raw_val)) return 0.0f;
    
    if (raw_val == 0x7F) return 0.0f;
    
    if (raw_val <= 0x24) {
        return 18.0f - (raw_val * 0.5f);
    } else {
        return -(raw_val - 0x24) * 0.5f;
    }
}

bool tlv320_config_micbias(tlv320dac3100_t *dev, bool power_down, bool always_on,
                            tlv320_micbias_volt_t voltage) {
    if (!set_page(dev, 1)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_MICBIAS, 7, 1, power_down ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_MICBIAS, 3, 1, always_on ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_MICBIAS, 0, 2, voltage)) return false;
    
    return true;
}

bool tlv320_set_input_common_mode(tlv320dac3100_t *dev, bool ain1_cm, bool ain2_cm) {
    if (!set_page(dev, 1)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_INPUT_CM, 7, 1, ain1_cm ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_INPUT_CM, 6, 1, ain2_cm ? 1 : 0)) return false;
    
    return true;
}

bool tlv320_config_delay_divider(tlv320dac3100_t *dev, bool use_mclk, uint8_t divider) {
    if (!set_page(dev, 3)) return false;
    
    if (!write_bits(dev, TLV320DAC3100_REG_TIMER_MCLK_DIV, 7, 1, use_mclk ? 1 : 0)) return false;
    if (!write_bits(dev, TLV320DAC3100_REG_TIMER_MCLK_DIV, 0, 7, divider)) return false;
    
    return true;
}

// Low-level register access
uint8_t tlv320_read_register(tlv320dac3100_t *dev, uint8_t page, uint8_t reg) {
    set_page(dev, page);
    uint8_t val;
    read_register(dev, reg, &val);
    return val;
}

bool tlv320_write_register(tlv320dac3100_t *dev, uint8_t page, uint8_t reg, uint8_t value) {
    if (!set_page(dev, page)) return false;
    return write_register(dev, reg, value);
}
