#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <vector>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/rand.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"

#include "pico_tlv320dac3100.h"
// #include "i2s_master_output.h"
#include "pico_ssd1681.h"

#include "pindefs.h"
#include "keypad.h"
#include "audio.h"
// #include "tlv320aic3100.h"

//global variables
#define ADDRESS_I2C_AUDIOAMP 0b0011000

//key names for printing
char key_names[23][6] = {
    "Null", "Up", "Down", "Left", "Right", "Lock", "Side1", "Side2",
    "PTT", "Back", "Enter", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "*", "#"
};

//Audio Amplifier object
// TLV320AIC3100 audio_amp;
audio_config_t audio_cfg;


/// I2C1 Initialization
void I2C1_init(){
    // Force-reset the I2C1 peripheral in case the Wi-Fi core or bootloader locked it
    i2c1_hw->enable = 0;
    sleep_us(10);

    // Set up GPIO pins for I2C1
    gpio_set_function(I2C1_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL, GPIO_FUNC_I2C);

    // Initialize I2C1 at 100kHz
    i2c_init(i2c1, 100000);
}

void I2C1_scan_bus(){
    printf("Scanning I2C1 bus...\n");
    for (uint8_t address = 1; address < 127; address++) {
        uint8_t txdata[] = {0x00};
        int ret = i2c_write_blocking(i2c1, address, txdata, 1, false);
        if (ret >= 0) {
            printf("Found device at address 0x%02X\n", address);
        }
    }
    printf("I2C1 scan complete.\n");
}

//initialize battery voltage reading
void init_battery_voltage(){
    adc_init();
    adc_gpio_init(V_BAT);
}
void init_volume_pot(){
    adc_init();
    adc_gpio_init(POT_VOLUME);
}

//returns battery voltage in volts
float get_battery_voltage(){
    adc_select_input(V_BAT-40); //V_BAT is ADC7, ADC input is 0 indexed
    return (float)adc_read()*.003791; //conversion factor for voltage divider and ADC step size (127/27)*(3.3/4095)
}
uint8_t get_volume_pot(){
    adc_select_input(POT_VOLUME-40); //POT_VOLUME is ADC5, ADC input is 0 indexed
    return 99-((uint8_t)((float)adc_read()*0.02442));
}

void init_encoder(){
    //init encoder pins
    gpio_init(BTN_ENC_A);
    gpio_set_dir(BTN_ENC_A, GPIO_IN);
    gpio_init(BTN_ENC_B);
    gpio_set_dir(BTN_ENC_B, GPIO_IN);
}


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
    ssd1681_update(SSD1681_UPDATE_CLEAN_FULL_AGGRESSIVE);
}


void init_all(){
    //initialize all necessary pins

    //init I2C1
    I2C1_init();

    audio_init(&audio_cfg, AUDIOAMP_RESET, AUDIOAMP_MASTERCLK, i2c1, ADDRESS_I2C_AUDIOAMP); //initialize audio amp

    display_init(); //initialize e-ink display

    Keypad::init(); //initialize keypad

    //init status LED
    gpio_init(LED_STATUS);
    gpio_set_dir(LED_STATUS, GPIO_OUT);
    gpio_put(LED_STATUS, 1);

    //init battery voltage reading
    init_battery_voltage();

    //init encoder pins
    init_encoder();
}

//what will run on core 0
void core_0() {

    uint64_t loop_time_target_us = 1000; //target loop time of 1ms

    printf("Core 0 launched\n");

    bool led_state = false;
    gpio_put(LED_STATUS, 1);

    uint8_t encoder_state = 0;
    uint8_t current_volume = 0;
    uint8_t last_volume = 0;

    uint16_t counter = 0;
    uint16_t slowest_loop_time_us = 0;
    float rolling_average_loop_time_us = 0.0f;
    uint64_t loop_start_us = time_us_64();

    while (true) {
        //toggle LED
        if(!(counter%500)){
            led_state = !led_state;
            gpio_put(LED_STATUS, led_state);
        }
        

        if (!(counter%10)){
            //scan buttons
            Keypad::keypad_poll();
            std::vector<Keys> pressed_keys = Keypad::get_buttons_pressed();
            for(uint8_t i=0; i<pressed_keys.size(); i++){
                printf("Key Pressed: %s\n", key_names[pressed_keys[i]]);
                //play beep on button press
                int8_t vol_db = (int8_t)(((float)current_volume * 0.619191) - 61.0f);
                audio_beep(&audio_cfg, 4000, 20, vol_db);
                ssd1681_clear(SSD1681_COLOR_BLACK);
                // ssd1681_write_buffer_and_update_if_ready(SSD1681_UPDATE_FAST_FULL);
            }   
            
        }

        //process encoder
        encoder_state=((encoder_state<<1) | gpio_get(BTN_ENC_A)) & 0b111;//store the last three states of the encoder A pin
        if(encoder_state==0b100){
            if(gpio_get(BTN_ENC_B)){
                printf("Channel +\n");
            } else{
                printf("Channel -\n");
            }
            encoder_state=0;
        }

        //read volume pot
        if (!(counter%200)){
            current_volume = get_volume_pot();
            if(abs(current_volume-last_volume)>2){
                last_volume = current_volume;
                printf("Volume: %d\n", current_volume);
                audio_amp_set_volume(&audio_cfg, current_volume);
            }
        }       

        if (!(counter%1000)){
            char voltage_string[6];
            sprintf(voltage_string, "%.2fV", get_battery_voltage());
            uint8_t x=(uint8_t)(get_rand_32() % 200);
            uint8_t y=(uint8_t)(get_rand_32() % 200);
            // for(uint16_t i=0; i<200*200; i++){
            //     ssd1681_write_point(SSD1681_COLOR_BLACK, i%200, i/200, get_rand_32() & 0x01);
            // }
            ssd1681_fill_rect(SSD1681_COLOR_BLACK, x, y, x+2, y+2, 1);
            ssd1681_draw_string(SSD1681_COLOR_BLACK, 40, 50, "HT-15", 5, 1, SSD1681_FONT_24);
            ssd1681_draw_string(SSD1681_COLOR_BLACK, 40, 75, "HT-15", 5, 1, SSD1681_FONT_12);
            ssd1681_draw_string(SSD1681_COLOR_BLACK, 10, 10, voltage_string, 5, 1, SSD1681_FONT_8);
            // for (uint8_t i=1; i<14; i++){
            //     char string[15];
            //     sprintf(string, "%d: aAbBcC", i);
            //     ssd1681_draw_string(SSD1681_COLOR_BLACK, 10, 10+(i*(i-1)), string, 10, 1, i);
            // }

            // ssd1681_fill_rect(SSD1681_COLOR_BLACK, 10, 10, 20, 20, 1);
            // ssd1681_fill_rect(SSD1681_COLOR_BLACK, 20, 20, 40, 40, 1);
            ssd1681_write_buffer_and_update_if_ready(SSD1681_UPDATE_FAST_PARTIAL);
        }

        //every 10 seconds
        if (!counter){
            printf("Battery Voltage: %.2fV\n", get_battery_voltage());
            printf("Max CPU time last 10 seonds: %.2f%% (%d us)\n", 100.0f*(float)slowest_loop_time_us/(float)loop_time_target_us, slowest_loop_time_us);
            slowest_loop_time_us = 0;
            printf("Rolling Average CPU time: %.2f%% (%.2f us)\n", 100.0f*((float)rolling_average_loop_time_us/(float)loop_time_target_us), rolling_average_loop_time_us);
            
        }

        //manage counter
        counter++;
        if(counter>=10000){
            counter = 0;
        }
        loop_start_us = time_us_64()-loop_start_us;
        sleep_us(loop_time_target_us>loop_start_us ? loop_time_target_us-loop_start_us : 0);
        if (loop_start_us > slowest_loop_time_us){
            slowest_loop_time_us = loop_start_us;
        }
        rolling_average_loop_time_us = (rolling_average_loop_time_us  *0.999f) + ((float)loop_start_us * 0.001f);
        loop_start_us = time_us_64();

    }
}

void core_1() {
    printf("Core 1 launched\n");
    while(true){
        sleep_ms(1000);
    }
}

int main(){
    
    stdio_init_all();
    sleep_ms(1000);
    init_all();

    //play startup beep
    audio_beep(&audio_cfg, 1000, 20, -30);
    sleep_ms(100);
    audio_beep(&audio_cfg, 1000, 20, -30);

    printf("Device Initalized!\n");

    multicore_reset_core1();
    multicore_launch_core1(core_1);

    core_0();

}