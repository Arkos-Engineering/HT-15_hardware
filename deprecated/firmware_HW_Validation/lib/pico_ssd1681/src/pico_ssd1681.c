/**
 * SSD1681 E-Paper Display Driver for Raspberry Pi Pico
 * Direct Pico SDK implementation - no HAL abstraction
 * 
 * Copyright (c) 2026 OpenCode
 * SPDX-License-Identifier: MIT
 */

#include "pico_ssd1681.h"
#include "pico_ssd1681_font.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include <string.h>
#include <stdio.h>

#define DISPLAY_WIDTH  200
#define DISPLAY_HEIGHT 200
#define BYTES_PER_ROW  (DISPLAY_WIDTH / 8)

/* Global state */
static struct {
    ssd1681_config_t config;
    bool initialized;
    uint8_t dc_state;  /* For 3-wire mode */
    spi_inst_t *spi;
    uint8_t black_gram[DISPLAY_HEIGHT][BYTES_PER_ROW];
    uint8_t red_gram[DISPLAY_HEIGHT][BYTES_PER_ROW];
} g_ssd1681 = {0};

/* SPI commands */
#define CMD_DRIVER_OUTPUT_CONTROL     0x01
#define CMD_GATE_DRIVING_VOLTAGE      0x03
#define CMD_SOURCE_DRIVING_VOLTAGE    0x04
#define CMD_BOOSTER_SOFT_START        0x0C
#define CMD_DEEP_SLEEP_MODE           0x10
#define CMD_DATA_ENTRY_MODE           0x11
#define CMD_SW_RESET                  0x12
#define CMD_MASTER_ACTIVATION         0x20
#define CMD_DISPLAY_UPDATE_CONTROL    0x21
#define CMD_DISPLAY_UPDATE_CONTROL_2  0x22
#define CMD_WRITE_RAM_BW              0x24
#define CMD_WRITE_RAM_RED             0x26
#define CMD_VCOM_REGISTER             0x2C
#define CMD_WRITE_LUT_REGISTER        0x32
#define CMD_SET_RAM_X_ADDRESS_COUNTER 0x4E
#define CMD_SET_RAM_Y_ADDRESS_COUNTER 0x4F
#define CMD_SET_RAM_X_START_END       0x44
#define CMD_SET_RAM_Y_START_END       0x45

/* Static functions */
static void ssd1681_spi_write_byte(uint8_t data);
static void ssd1681_write_cmd(uint8_t cmd);
static void ssd1681_write_data(uint8_t data);
static void ssd1681_write_data_buf(const uint8_t *data, uint16_t len);
static void ssd1681_reset(void);
static void ssd1681_wait_busy(void);
static void ssd1681_set_window(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end);
static void ssd1681_set_cursor(uint8_t x, uint8_t y);
static void ssd1681_set_spi_mode_and_clk(ssd1681_config_t *config);

/**
 * @brief Write a byte via SPI (handles both 3-wire and 4-wire)
 */
static void ssd1681_spi_write_byte(uint8_t data)
{
    if (g_ssd1681.config.spi_mode == SSD1681_SPI_3WIRE) {
        /* 3-wire: Send 9-bit frame (D/C + 8 data bits) */
        uint16_t frame = ((uint16_t)g_ssd1681.dc_state << 8) | data;
        
        /* Wait for TX FIFO space */
        while (!spi_is_writable(g_ssd1681.spi)) tight_loop_contents();
        
        /* Write 9-bit frame directly to hardware register */
        spi_get_hw(g_ssd1681.spi)->dr = frame;
        
        /* Wait for transmission complete */
        while (spi_is_busy(g_ssd1681.spi)) tight_loop_contents();
    } else {
        /* 4-wire: Standard 8-bit SPI */
        spi_write_blocking(g_ssd1681.spi, &data, 1);
    }
}

/**
 * @brief Write command
 */
static void ssd1681_write_cmd(uint8_t cmd)
{
    ssd1681_set_spi_mode_and_clk(&g_ssd1681.config);  /* Ensure correct SPI mode is set */
    if (g_ssd1681.config.spi_mode == SSD1681_SPI_3WIRE) {
        g_ssd1681.dc_state = 0;  /* Command mode */
    } else {
        gpio_put(g_ssd1681.config.pin_dc, 0);  /* D/C = 0 (command) */
    }
    
    gpio_put(g_ssd1681.config.pin_cs, 0);  /* CS = 0 */
    ssd1681_spi_write_byte(cmd);
    gpio_put(g_ssd1681.config.pin_cs, 1);  /* CS = 1 */
}

/**
 * @brief Write data byte
 */
static void ssd1681_write_data(uint8_t data)
{
    ssd1681_set_spi_mode_and_clk(&g_ssd1681.config);  /* Ensure correct SPI mode is set */
    if (g_ssd1681.config.spi_mode == SSD1681_SPI_3WIRE) {
        g_ssd1681.dc_state = 1;  /* Data mode */
    } else {
        gpio_put(g_ssd1681.config.pin_dc, 1);  /* D/C = 1 (data) */
    }
    
    gpio_put(g_ssd1681.config.pin_cs, 0);
    ssd1681_spi_write_byte(data);
    gpio_put(g_ssd1681.config.pin_cs, 1);
}

/**
 * @brief Write data buffer
 */
static void ssd1681_write_data_buf(const uint8_t *data, uint16_t len)
{
    ssd1681_set_spi_mode_and_clk(&g_ssd1681.config);  /* Ensure correct SPI mode is set */
    if (g_ssd1681.config.spi_mode == SSD1681_SPI_3WIRE) {
        g_ssd1681.dc_state = 1;
    } else {
        gpio_put(g_ssd1681.config.pin_dc, 1);
    }
    
    gpio_put(g_ssd1681.config.pin_cs, 0);
    for (uint16_t i = 0; i < len; i++) {
        ssd1681_spi_write_byte(data[i]);
    }
    gpio_put(g_ssd1681.config.pin_cs, 1);
}

/**
 * @brief Reset the display
 */
static void ssd1681_reset(void)
{
    gpio_put(g_ssd1681.config.pin_rst, 1);
    sleep_ms(10);
    gpio_put(g_ssd1681.config.pin_rst, 0);
    sleep_ms(10);
    gpio_put(g_ssd1681.config.pin_rst, 1);
    sleep_ms(10);
}

/**
 * @brief Wait for display to be ready
 */
static void ssd1681_wait_busy(void)
{

    int32_t timeout = 1000000;  // 10 second timeout

    while (gpio_get(g_ssd1681.config.pin_busy)) {
        sleep_us(10);
        if (--timeout <= 0) {
            break;
        }
    }

    sleep_us(100); // Extra delay to ensure ready. This apparently is a known issue.
}

/**
 * @brief Set RAM window
 */
static void ssd1681_set_window(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end)
{
    /* Set RAM X address */
    ssd1681_write_cmd(CMD_SET_RAM_X_START_END);
    ssd1681_write_data(x_start / 8);
    ssd1681_write_data(x_end / 8);
    
    /* Set RAM Y address */
    ssd1681_write_cmd(CMD_SET_RAM_Y_START_END);
    ssd1681_write_data(y_start & 0xFF);
    ssd1681_write_data((y_start >> 8) & 0xFF);
    ssd1681_write_data(y_end & 0xFF);
    ssd1681_write_data((y_end >> 8) & 0xFF);
}

/**
 * @brief Set RAM cursor
 */
static void ssd1681_set_cursor(uint8_t x, uint8_t y)
{
    ssd1681_write_cmd(CMD_SET_RAM_X_ADDRESS_COUNTER);
    ssd1681_write_data(x / 8);
    
    ssd1681_write_cmd(CMD_SET_RAM_Y_ADDRESS_COUNTER);
    ssd1681_write_data(y & 0xFF);
    ssd1681_write_data((y >> 8) & 0xFF);
}

/**
 * @brief Get default 4-wire configuration
 */
void ssd1681_get_default_config_4wire(ssd1681_config_t *config)
{
    config->spi_mode = SSD1681_SPI_4WIRE;
    config->spi_port = 0;
    config->pin_mosi = 19;
    config->pin_sck = 18;
    config->pin_cs = 17;
    config->pin_dc = 20;
    config->pin_rst = 21;
    config->pin_busy = 22;
    config->spi_baudrate = 4000000;
}

/**
 * @brief Get default 3-wire configuration
 */
void ssd1681_get_default_config_3wire(ssd1681_config_t *config)
{
    config->spi_mode = SSD1681_SPI_3WIRE;
    config->spi_port = 0;
    config->pin_mosi = 19;
    config->pin_sck = 18;
    config->pin_cs = 17;
    config->pin_dc = 0;  /* Unused in 3-wire mode */
    config->pin_rst = 21;
    config->pin_busy = 22;
    config->spi_baudrate = 4000000;
}

static void ssd1681_set_spi_mode_and_clk(ssd1681_config_t *config) {
    if(spi_get_baudrate(g_ssd1681.spi) != config->spi_baudrate){
        spi_set_baudrate(g_ssd1681.spi, config->spi_baudrate);
    }

    if (config->spi_mode == SSD1681_SPI_3WIRE) {
        /* Configure for 9-bit frames */
        spi_hw_t *spi_hw = spi_get_hw(g_ssd1681.spi);
        hw_clear_bits(&spi_hw->cr1, SPI_SSPCR1_SSE_BITS);  /* Disable SPI */
        spi_hw->cr0 = (8 << SPI_SSPCR0_DSS_LSB) |  /* 9-bit (DSS = bits - 1) */
                      (0 << SPI_SSPCR0_FRF_LSB) |  /* SPI format */
                      (0 << SPI_SSPCR0_SPO_LSB) |  /* CPOL = 0 */
                      (0 << SPI_SSPCR0_SPH_LSB);   /* CPHA = 0 */
        hw_set_bits(&spi_hw->cr1, SPI_SSPCR1_SSE_BITS);  /* Re-enable SPI */
        // spi_set_format(
        //         g_ssd1681.spi,          // SPI instance (spi0 or spi1)
        //         9,                      // data_bits: 9 (valid range 4–16)
        //         SPI_CPOL_0,             // clock polarity = 0 (matches your original SPO=0)
        //         SPI_CPHA_0,             // clock phase = 0     (matches your original SPH=0)
        //         SPI_MSB_FIRST           // order: MSB first (standard for displays; the 9th bit is sent first as D/C)
        //     );
    } else {
        /* Standard 8-bit SPI */
        spi_set_format(g_ssd1681.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    }
}

/**
 * @brief Initialize the display
 */
int ssd1681_init(const ssd1681_config_t *config)
{
    if (!config) return -1;
    if (g_ssd1681.initialized) return -2;
    
    memcpy(&g_ssd1681.config, config, sizeof(ssd1681_config_t));
    g_ssd1681.dc_state = 0;
    
    /* Get SPI instance */
    g_ssd1681.spi = (config->spi_port == 0) ? spi0 : spi1;
    
    /* Initialize SPI */
    uint32_t actual_baud = spi_init(g_ssd1681.spi, config->spi_baudrate);
    printf("SSD1681: Requested SPI baudrate %u Hz, actual %u Hz\n", config->spi_baudrate, actual_baud);
    if (actual_baud == 0) return -3;
    
    ssd1681_set_spi_mode_and_clk(&g_ssd1681.config);  /* Set initial SPI mode and clock */

    /* Configure GPIO pins */
    gpio_set_function(config->pin_mosi, GPIO_FUNC_SPI);
    gpio_set_function(config->pin_sck, GPIO_FUNC_SPI);
    
    gpio_init(config->pin_cs);
    gpio_set_dir(config->pin_cs, GPIO_OUT);
    gpio_put(config->pin_cs, 1);
    
    if (config->spi_mode == SSD1681_SPI_4WIRE) {
        gpio_init(config->pin_dc);
        gpio_set_dir(config->pin_dc, GPIO_OUT);
        gpio_put(config->pin_dc, 0);
    }
    
    gpio_init(config->pin_rst);
    gpio_set_dir(config->pin_rst, GPIO_OUT);
    gpio_put(config->pin_rst, 1);
    
    gpio_init(config->pin_busy);
    gpio_set_dir(config->pin_busy, GPIO_IN);
    gpio_pull_down(config->pin_busy);
    
    /* Reset display */
    ssd1681_reset();
    sleep_ms(10);
    ssd1681_wait_busy();
    
    /* Initialize display */
    ssd1681_write_cmd(CMD_SW_RESET);
    sleep_ms(10);
    ssd1681_wait_busy();
    
    /* Driver output control */
    ssd1681_write_cmd(CMD_DRIVER_OUTPUT_CONTROL);
    ssd1681_write_data(0xC7);  /* 200 - 1 */
    ssd1681_write_data(0x00);
    ssd1681_write_data(0x02);

    // ssd1681_set_soft_start(SSD1681_SOFTSTART_DRIVE_STRENGTH_0, SSD1681_SOFTSTART_TIME_40MS, SSD1681_SOFTSTART_MIN_OFF_4_6);
    
    /* Data entry mode */
    ssd1681_write_cmd(CMD_DATA_ENTRY_MODE);
    ssd1681_write_data(0x01);  /* Y decrement, X increment */
    
    /* Set window */
    ssd1681_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    
    /* Border waveform */
    ssd1681_write_cmd(0x3C);
    ssd1681_write_data(0x05);
    
    /* Temperature sensor */
    ssd1681_write_cmd(0x18);
    ssd1681_write_data(0x80);  /* Internal sensor */
    
    /* Display update control */
    // ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
    // ssd1681_write_data(0x00);
    // ssd1681_write_data(0x80);
    // ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
    // ssd1681_write_data(0xFF);
    
    /* Master activation */
    // ssd1681_write_cmd(CMD_MASTER_ACTIVATION);
    ssd1681_wait_busy();
    
    /* Clear framebuffers */
    memset(g_ssd1681.black_gram, 0xFF, sizeof(g_ssd1681.black_gram));
    memset(g_ssd1681.red_gram, 0xFF, sizeof(g_ssd1681.red_gram));
    
    g_ssd1681.initialized = true;
    return 0;
}

/**
 * @brief Deinitialize the display
 */
void ssd1681_deinit(void)
{
    if (!g_ssd1681.initialized) return;
    
    /* Deep sleep */
    ssd1681_write_cmd(CMD_DEEP_SLEEP_MODE);
    ssd1681_write_data(0x01);
    
    /* Deinit SPI and GPIO */
    spi_deinit(g_ssd1681.spi);
    gpio_deinit(g_ssd1681.config.pin_cs);
    if (g_ssd1681.config.spi_mode == SSD1681_SPI_4WIRE) {
        gpio_deinit(g_ssd1681.config.pin_dc);
    }
    gpio_deinit(g_ssd1681.config.pin_rst);
    gpio_deinit(g_ssd1681.config.pin_busy);
    
    g_ssd1681.initialized = false;
}

/**
 * @brief Clear the display
 */
int ssd1681_clear(ssd1681_color_t color)
{
    if (!g_ssd1681.initialized) return -1;
    
    uint8_t *gram = (color == SSD1681_COLOR_BLACK) ? 
                    &g_ssd1681.black_gram[0][0] : &g_ssd1681.red_gram[0][0];
    
    memset(gram, 0xFF, DISPLAY_HEIGHT * BYTES_PER_ROW);

    // Write to RAM
    // ssd1681_set_cursor(0, 0);
    // ssd1681_write_cmd((color == SSD1681_COLOR_BLACK) ? CMD_WRITE_RAM_BW : CMD_WRITE_RAM_RED);
    // ssd1681_write_data_buf(gram, DISPLAY_HEIGHT * BYTES_PER_ROW);

    // ssd1681_update();
    
    return 0;
}

/**
 * @brief Write internal buffer to display RAM
 */
int ssd1681_write_buffer(ssd1681_color_t color)
{
    if (!g_ssd1681.initialized) return -1;
    
    uint8_t *gram = (color == SSD1681_COLOR_BLACK) ? 
                    &g_ssd1681.black_gram[0][0] : &g_ssd1681.red_gram[0][0];

    ssd1681_wait_busy();
    
    /* Set write window to full display */
    ssd1681_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    
    /* Set cursor to start */
    // ssd1681_set_cursor(0, DISPLAY_HEIGHT - 1);
    ssd1681_set_cursor(0, 0);
    
    /* Write buffer to display RAM */
    ssd1681_write_cmd((color == SSD1681_COLOR_BLACK) ? CMD_WRITE_RAM_BW : CMD_WRITE_RAM_RED);
    ssd1681_write_data_buf(gram, DISPLAY_HEIGHT * BYTES_PER_ROW);
    
    return 0;
}

/**
 * @brief Set soft start parameters 
 */
int ssd1681_set_soft_start(ssd1681_softstart_drive_strength_t strength,  ssd1681_softstart_time_t time, ssd1681_softstart_min_off_time_t min_off)
{
    if (!g_ssd1681.initialized) return -1;
    
    ssd1681_write_cmd(CMD_BOOSTER_SOFT_START);
    for (int i = 0; i < 3; i++) {
        ssd1681_write_data(strength << 4 | min_off);
    }
    ssd1681_write_data(time);

    return 0;
}

/**
 * @brief Update the display if display is ready
 * @return 0 on update, -1 if display is busy, -2 if invalid update type
 * @param update_type Update type (see ssd1681_update_type_t in header file)
 */
int ssd1681_write_buffer_and_update_if_ready(uint8_t update_type)
{
    if (!g_ssd1681.initialized) return -1;

    if (gpio_get(g_ssd1681.config.pin_busy)) {
        return -1; // Display is busy
    }


    if (update_type == SSD1681_UPDATE_CLEAN_FULL) {
        ssd1681_write_buffer(SSD1681_COLOR_BLACK);

        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xF6);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);
        
    } else if(update_type == SSD1681_UPDATE_FAST_PARTIAL) {
        ssd1681_write_buffer(SSD1681_COLOR_BLACK);

        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xFE);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);

    } else if(update_type == SSD1681_UPDATE_FAST_FULL) {
        ssd1681_write_cmd(CMD_WRITE_RAM_BW);
        for (uint16_t i = 0; i < sizeof(g_ssd1681.black_gram); i++) {
            ssd1681_write_data(0xFF);
        }

        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xFE);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);
    
        ssd1681_wait_busy();

        ssd1681_write_buffer(SSD1681_COLOR_BLACK);

        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xFE);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);

    } else if(update_type == SSD1681_UPDATE_CLEAN_FULL_AGGRESSIVE) {
        ssd1681_write_buffer(SSD1681_COLOR_BLACK);

        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xF6);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);
        ssd1681_wait_busy();
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xF6);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);
    } else {
        return -2; // Invalid update type
    }
    return 0;
}


/**
 * @brief Update the display
 */
int ssd1681_update(uint8_t update_type)
{
    if (!g_ssd1681.initialized) return -1;

    ssd1681_wait_busy();


    if (update_type == SSD1681_UPDATE_CLEAN_FULL) {
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xF6);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);
        
    } else if(update_type == SSD1681_UPDATE_FAST_PARTIAL) {
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xFE);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);

    } else if(update_type == SSD1681_UPDATE_FAST_FULL) {
        return -3; // Not supported in this function

    } else if(update_type == SSD1681_UPDATE_CLEAN_FULL_AGGRESSIVE) {
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xF6);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);
        ssd1681_wait_busy();
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL);
        ssd1681_write_data(0x00);
        ssd1681_write_data(0x80);
        ssd1681_write_cmd(CMD_DISPLAY_UPDATE_CONTROL_2);
        ssd1681_write_data(0xF6);
        ssd1681_write_cmd(CMD_MASTER_ACTIVATION);
    } else {
        return -2; // Invalid update type
    }
    return 0;
}

/**
 * @brief Write a point
 */
int ssd1681_write_point(ssd1681_color_t color, uint8_t x, uint8_t y, uint8_t data)
{
    if (!g_ssd1681.initialized) return -1;
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return -2;
    
    uint8_t *gram = (color == SSD1681_COLOR_BLACK) ? 
                    &g_ssd1681.black_gram[0][0] : &g_ssd1681.red_gram[0][0];
    
    uint16_t byte_index = (DISPLAY_HEIGHT - 1 - y) * BYTES_PER_ROW + (x / 8);
    uint8_t bit_index = 7 - (x % 8);
    
    if (data) {
        gram[byte_index] &= ~(1 << bit_index);
    } else {
        gram[byte_index] |= (1 << bit_index);
    }
    
    return 0;
}

/**
 * @brief Read a point
 */
int ssd1681_read_point(ssd1681_color_t color, uint8_t x, uint8_t y, uint8_t *data)
{
    if (!g_ssd1681.initialized) return -1;
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return -2;
    if (!data) return -3;
    
    uint8_t *gram = (color == SSD1681_COLOR_BLACK) ? 
                    &g_ssd1681.black_gram[0][0] : &g_ssd1681.red_gram[0][0];
    
    uint16_t byte_index = y * BYTES_PER_ROW + (x / 8);
    uint8_t bit_index = 7 - (x % 8);
    
    *data = (gram[byte_index] & (1 << bit_index)) ? 0 : 1;
    
    return 0;
}

/**
 * @brief Draw string (simplified - needs font data)
 */
int ssd1681_draw_string(ssd1681_color_t color, uint8_t x, uint8_t y,
                        const char *str, uint16_t len, uint8_t data,
                        uint8_t font_size)
{
    if (!g_ssd1681.initialized) return -1;
    if (!str) return -2;


    for(uint16_t i = 0; i < len; i++) {
        char c = str[i];
        if (c > 127) continue;  /* Skip unsupported characters */
        
        const char *char_bitmap = font_basic_8x8[(uint8_t)c];
        
        for(uint8_t row = 0; row < font_size; row++) {
            for (uint8_t col = 0; col < font_size; col++) {
                ssd1681_write_point(color, x+col, y+row, char_bitmap[(row*FONT_BASIC_SIZE)/font_size] & ( 1 << ((col*FONT_BASIC_SIZE/font_size))));
            }
        }

        x += font_size;  /* Move to next character position */
        if (x + font_size > DISPLAY_WIDTH) {
            x = 0;
            y += font_size;
            if (y + font_size > DISPLAY_HEIGHT) {
                break;  /* No more space */
            }
        }
    }

    
    return 0;
}

/**
 * @brief Fill rectangle
 */
int ssd1681_fill_rect(ssd1681_color_t color, uint8_t left, uint8_t top,
                      uint8_t right, uint8_t bottom, uint8_t data)
{
    if (!g_ssd1681.initialized) return -1;
    if (left >= DISPLAY_WIDTH || top >= DISPLAY_HEIGHT) return -2;
    if (right >= DISPLAY_WIDTH || bottom >= DISPLAY_HEIGHT) return -3;
    if (left > right || top > bottom) return -4;
    
    for (uint8_t y = top; y <= bottom; y++) {
        for (uint8_t x = left; x <= right; x++) {
            ssd1681_write_point(color, x, y, data);
        }
    }
    
    return 0;
}

/**
 * @brief Draw picture
 */
int ssd1681_draw_picture(ssd1681_color_t color, uint8_t left, uint8_t top,
                         uint8_t right, uint8_t bottom, const uint8_t *img)
{
    if (!g_ssd1681.initialized) return -1;
    if (!img) return -2;
    if (left >= DISPLAY_WIDTH || top >= DISPLAY_HEIGHT) return -3;
    if (right >= DISPLAY_WIDTH || bottom >= DISPLAY_HEIGHT) return -4;
    if (left > right || top > bottom) return -5;
    
    uint16_t width = right - left + 1;
    uint16_t height = bottom - top + 1;
    uint16_t bytes_per_line = (width + 7) / 8;
    
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            uint16_t img_byte = y * bytes_per_line + (x / 8);
            uint8_t img_bit = 7 - (x % 8);
            uint8_t pixel = (img[img_byte] & (1 << img_bit)) ? 1 : 0;
            ssd1681_write_point(color, left + x, top + y, pixel);
        }
    }
    
    return 0;
}
