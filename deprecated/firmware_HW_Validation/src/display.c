/**
 * @file display.c
 * @brief Display related functions for the HT-15
 */
#include <stdio.h>
#include "pindefs.h"
#include "pico/stdlib.h"

#include "pico_ssd1681.h"


void display_init(){
    ssd1681_config_t display_config;
    ssd1681_get_default_config_3wire(&display_config);
    display_config.spi_port = 1;
    display_config.spi_baudrate = 4000000; //10 MHz for some reason.
    display_config.spi_mode = SSD1681_SPI_3WIRE;
    display_config.pin_mosi = SPI1_SDI;
    display_config.pin_sck = SPI1_CLK;
    display_config.pin_cs = DISPLAY_CS;
    display_config.pin_rst = DISPLAY_RESET;
    display_config.pin_busy = DISPLAY_BUSY;


    if(ssd1681_init(&display_config)!=0){
        printf("Error initializing display!\n");
        return;
    } else{
        printf("Display initialized successfully!\n");
    }

    sleep_ms(100);

    // ssd1681_draw_string(SSD1681_COLOR_BLACK, 10, 10, "HT-15 Test", 2, 1, SSD1681_FONT_24);
    // for(int x=0; x<200; x+=2){
    //     for(int y=0; y<200; y+=2){
    //         ssd1681_write_point(SSD1681_COLOR_BLACK, x, y, 1);
    //     }
    // }
    // ssd1681_write_point(SSD1681_COLOR_BLACK, 50, 50, 1);

    // ssd1681_fill_rect(SSD1681_COLOR_BLACK, 0, 0, 199, 199, 0);
    ssd1681_clear(SSD1681_COLOR_BLACK);
    ssd1681_write_buffer(SSD1681_COLOR_BLACK);
    ssd1681_update(SSD1681_UPDATE_CLEAN_FULL);
    ssd1681_clear(SSD1681_COLOR_BLACK);
    ssd1681_write_buffer(SSD1681_COLOR_BLACK);
    ssd1681_update(SSD1681_UPDATE_CLEAN_FULL);
}
