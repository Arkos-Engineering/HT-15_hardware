/**
 * SSD1681 E-Paper Display Example
 * Demonstrates both 3-wire and 4-wire SPI modes
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico_ssd1681.h"

int main(void)
{
    ssd1681_config_t config;
    int result;
    
    /* Initialize stdio */
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n=== SSD1681 E-Paper Display Example ===\n");
    
    /* Choose mode: 3-wire or 4-wire */
#ifdef USE_3WIRE_SPI
    printf("Using 3-wire SPI mode (9-bit frames, no D/C pin)\n");
    ssd1681_get_default_config_3wire(&config);
#else
    printf("Using 4-wire SPI mode (separate D/C pin)\n");
    ssd1681_get_default_config_4wire(&config);
#endif
    
    /* You can customize pins here */
    // config.pin_mosi = 19;
    // config.pin_sck = 18;
    // config.spi_baudrate = 8000000;  /* 8 MHz */
    
    printf("Initializing display...\n");
    result = ssd1681_init(&config);
    if (result != 0) {
        printf("ERROR: Init failed (%d)\n", result);
        return 1;
    }
    printf("Display initialized\n");
    
    /* Clear both color planes */
    printf("Clearing display...\n");
    ssd1681_clear(SSD1681_COLOR_BLACK);
    ssd1681_clear(SSD1681_COLOR_RED);
    ssd1681_update(1);
    
    /* Draw a rectangle */
    printf("Drawing rectangle...\n");
    ssd1681_fill_rect(SSD1681_COLOR_BLACK, 20, 20, 100, 80, 1);
    
    /* Draw a pattern */
    printf("Drawing pattern...\n");
    for (uint8_t y = 100; y < 120; y++) {
        for (uint8_t x = 20; x < 100; x++) {
            if ((x + y) % 2 == 0) {
                ssd1681_write_point(SSD1681_COLOR_RED, x, y, 1);
            }
        }
    }
    
    /* Draw a line */
    printf("Drawing line...\n");
    for (uint8_t i = 0; i < 50; i++) {
        ssd1681_write_point(SSD1681_COLOR_BLACK, 120 + i, 40 + i, 1);
    }
    
    /* Update display */
    printf("Updating display (this takes ~2-3 seconds)...\n");
    ssd1681_update(1);
    
    printf("\n=== Test Complete ===\n");
    printf("Display should show:\n");
    printf("- A black rectangle\n");
    printf("- A checkerboard pattern in red\n");
    printf("- A diagonal line\n");
    
    /* Main loop */
    while (1) {
        sleep_ms(1000);
    }
    
    return 0;
}
