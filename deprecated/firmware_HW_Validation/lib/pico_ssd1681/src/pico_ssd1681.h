/**
 * SSD1681 E-Paper Display Driver for Raspberry Pi Pico
 * Direct Pico SDK implementation - no HAL abstraction
 * 
 * Copyright (c) 2026 OpenCode
 * SPDX-License-Identifier: MIT
 */

#ifndef PICO_SSD1681_H
#define PICO_SSD1681_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI mode configuration
 */
typedef enum {
    SSD1681_SPI_4WIRE = 0,  /**< 4-wire SPI with separate D/C pin */
    SSD1681_SPI_3WIRE = 1,  /**< 3-wire SPI with 9-bit frames (no D/C pin) */
} ssd1681_spi_mode_t;

typedef enum {
    SSD1681_SOFTSTART_TIME_10MS = 0,
    SSD1681_SOFTSTART_TIME_20MS = 1,
    SSD1681_SOFTSTART_TIME_30MS = 2,
    SSD1681_SOFTSTART_TIME_40MS = 3,

} ssd1681_softstart_time_t;

typedef enum {
    SSD1681_SOFTSTART_MIN_OFF_2_6 = 0b0100,
    SSD1681_SOFTSTART_MIN_OFF_3_2 = 0b0101,
    SSD1681_SOFTSTART_MIN_OFF_3_9 = 0b0110,
    SSD1681_SOFTSTART_MIN_OFF_4_6 = 0b0111,
    SSD1681_SOFTSTART_MIN_OFF_5_4 = 0b1000,
    SSD1681_SOFTSTART_MIN_OFF_6_3 = 0b1001,
    SSD1681_SOFTSTART_MIN_OFF_7_3 = 0b1010,
    SSD1681_SOFTSTART_MIN_OFF_8_4 = 0b1011,
    SSD1681_SOFTSTART_MIN_OFF_9_8 = 0b1100,
    SSD1681_SOFTSTART_MIN_OFF_11_5 = 0b1101,
    SSD1681_SOFTSTART_MIN_OFF_13_8 = 0b1110,
    SSD1681_SOFTSTART_MIN_OFF_16_5 = 0b1111


} ssd1681_softstart_min_off_time_t;

typedef enum {
    SSD1681_SOFTSTART_DRIVE_STRENGTH_0 = 0,
    SSD1681_SOFTSTART_DRIVE_STRENGTH_1 = 1,
    SSD1681_SOFTSTART_DRIVE_STRENGTH_2 = 2,
    SSD1681_SOFTSTART_DRIVE_STRENGTH_3 = 3,
    SSD1681_SOFTSTART_DRIVE_STRENGTH_4 = 4,
    SSD1681_SOFTSTART_DRIVE_STRENGTH_5 = 5,
    SSD1681_SOFTSTART_DRIVE_STRENGTH_6 = 6,
    SSD1681_SOFTSTART_DRIVE_STRENGTH_7 = 7,


} ssd1681_softstart_drive_strength_t;

/**
 * @brief Pin configuration structure
 */
typedef struct {
    ssd1681_spi_mode_t spi_mode;  /**< SPI mode (3-wire or 4-wire) */
    uint8_t spi_port;             /**< SPI port: 0 or 1 */
    uint8_t pin_mosi;             /**< MOSI pin */
    uint8_t pin_sck;              /**< SCK pin */
    uint8_t pin_cs;               /**< CS pin */
    uint8_t pin_dc;               /**< D/C pin (unused in 3-wire mode) */
    uint8_t pin_rst;              /**< Reset pin */
    uint8_t pin_busy;             /**< Busy status pin */
    uint32_t spi_baudrate;        /**< SPI clock frequency in Hz */
} ssd1681_config_t;

/**
 * @brief Color selection
 */
typedef enum {
    SSD1681_COLOR_BLACK = 0,
    SSD1681_COLOR_RED = 1,
} ssd1681_color_t;

/**
 * @brief Display update type
 * @note UPDATE_FAST_PARTIAL: only draws new pixels (immidiate, ghosting likely)
 * @note UPDATE_CLEAN_FULL: calls display firmware to full refresh the display then draws all (~4 seconds, minimal ghosting)
 * @note UPDATE_FAST_FULL: flashes screen to black once, then draws all (~2 second, very little ghosting)
 * @note UPDATE_CLEAN_FULL_AGGRESSIVE: calls display firmware to full refresh the display twice then draws all (~8 seconds, zero ghosting)
 */
enum ssd1681_update_type_t{
    SSD1681_UPDATE_FAST_PARTIAL = 0b00,
    SSD1681_UPDATE_CLEAN_FULL = 0b01,
    SSD1681_UPDATE_FAST_FULL = 0b10,
    SSD1681_UPDATE_CLEAN_FULL_AGGRESSIVE = 0b11,
};

/**
 * @brief Font size. missing sizes can be passed as an int, however the below are known to look acceptable
 */
enum ssd1681_font_size_t{
    SSD1681_FONT_8 = 8,
    SSD1681_FONT_12 = 12,
    SSD1681_FONT_16 = 16,
    SSD1681_FONT_20 = 20,
    SSD1681_FONT_24 = 24,
    SSD1681_FONT_28 = 28,
    SSD1681_FONT_32 = 32,
    SSD1681_FONT_36 = 36,
    SSD1681_FONT_40 = 40,
    SSD1681_FONT_44 = 44,
    SSD1681_FONT_48 = 48,
};

/**
 * @brief Initialize the display
 * @param config Pin configuration
 * @return 0 on success, non-zero on error
 */
int ssd1681_init(const ssd1681_config_t *config);

/**
 * @brief Deinitialize the display
 */
void ssd1681_deinit(void);


/**
 * @brief Clear the display
 * @param color Color plane to clear
 * @return 0 on success
 */
int ssd1681_clear(ssd1681_color_t color);

/**
 * @brief Write internal buffer to display RAM
 * @param color Color plane to write
 * @return 0 on success
 */
int ssd1681_write_buffer(ssd1681_color_t color);

/**
 * @brief Write a single point
 * @param color Color plane
 * @param x X coordinate (0-199)
 * @param y Y coordinate (0-199)
 * @param data 1=on, 0=off
 * @return 0 on success
 */
int ssd1681_write_point(ssd1681_color_t color, uint8_t x, uint8_t y, uint8_t data);

/**
 * @brief Read a single point
 * @param color Color plane
 * @param x X coordinate
 * @param y Y coordinate
 * @param data Output: pixel value
 * @return 0 on success
 */
int ssd1681_read_point(ssd1681_color_t color, uint8_t x, uint8_t y, uint8_t *data);

/**
 * @brief Draw a string
 * @param color Color plane
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param len String length
 * @param data 1=visible, 0=invisible
 * @param font Font size
 * @return 0 on success
 */
int ssd1681_draw_string(ssd1681_color_t color, uint8_t x, uint8_t y, 
                        const char *str, uint16_t len, uint8_t data, 
                        uint8_t font);

/**
 * @brief Fill a rectangle
 * @param color Color plane
 * @param left Left X coordinate
 * @param top Top Y coordinate
 * @param right Right X coordinate
 * @param bottom Bottom Y coordinate
 * @param data Fill value (1=filled, 0=empty)
 * @return 0 on success
 */
int ssd1681_fill_rect(ssd1681_color_t color, uint8_t left, uint8_t top,
                      uint8_t right, uint8_t bottom, uint8_t data);

/**
 * @brief Draw an image
 * @param color Color plane
 * @param left Left X coordinate
 * @param top Top Y coordinate
 * @param right Right X coordinate
 * @param bottom Bottom Y coordinate
 * @param img Image buffer (1 bit per pixel, row-major)
 * @return 0 on success
 */
int ssd1681_draw_picture(ssd1681_color_t color, uint8_t left, uint8_t top,
                         uint8_t right, uint8_t bottom, const uint8_t *img);

/**
 * @brief Set soft start parameters
 * @param strength Drive strength
 * @param time Soft start time
 * @param min_off Minimum off time
 */
int ssd1681_set_soft_start(ssd1681_softstart_drive_strength_t strength,  ssd1681_softstart_time_t time, ssd1681_softstart_min_off_time_t min_off);

/**
 * @brief Update the display (refresh)
 * @return 0 on success
 */
int ssd1681_update(uint8_t update_type);

/**
 * @brief write buffer and update the display (refresh) only if the display is ready, otherwise do nothing
 * @return 0 on update, -1 if display is busy
 */
int ssd1681_write_buffer_and_update_if_ready(uint8_t update_type);

/**
 * @brief Get default configuration for 4-wire SPI
 * @param config Output configuration
 */
void ssd1681_get_default_config_4wire(ssd1681_config_t *config);

/**
 * @brief Get default configuration for 3-wire SPI
 * @param config Output configuration
 */
void ssd1681_get_default_config_3wire(ssd1681_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* pico_ssd1681.h */
