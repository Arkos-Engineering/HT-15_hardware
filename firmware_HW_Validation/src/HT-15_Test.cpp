#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <vector>

#include "pico/stdlib.h"
#include "pico/multicore.h"
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
    display_config.spi_baudrate = 1000000; //1 MHz
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

    ssd1681_clear(SSD1681_COLOR_BLACK);
    ssd1681_update();   

    // ssd1681_draw_string(SSD1681_COLOR_BLACK, 10, 10, "HT-15 Test", 2, 1, SSD1681_FONT_24);
    ssd1681_fill_rect(SSD1681_COLOR_BLACK, 20, 20, 100, 80, 1);
    // for(int x=0; x<200; x+=2){
    //     for(int y=0; y<200; y+=2){
    //         ssd1681_write_point(SSD1681_COLOR_BLACK, x, y, 1);
    //     }
    // }
    // ssd1681_write_point(SSD1681_COLOR_BLACK, 50, 50, 1);
    ssd1681_update();
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
    printf("Core 0 launched\n");


    bool led_state = false;
    gpio_put(LED_STATUS, 1);
   

    uint8_t encoder_state = 0;

    uint8_t current_volume = 0;
    uint8_t last_volume = 0;

    uint16_t counter = 0;
    printf("Starting main loop on core 0\n");
    while (true) {
        //toggle LED
        if(!(counter%500)){
            led_state = !led_state;
            gpio_put(LED_STATUS, led_state);
        }
        
        //read battery voltage every 10 seconds
        if (!counter){
            printf("Battery Voltage: %.2fV\n", get_battery_voltage());
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
                ssd1681_fill_rect(SSD1681_COLOR_BLACK, 20, 20, 100, 80, 1);
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

        if (!(counter%2000)){
            // I2C1_scan_bus();
        }

        //manage counter
        counter++;
        if(counter>=10000){
            counter = 0;
        }
        sleep_ms(1);
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
    audio_beep(&audio_cfg, 1000, 20, -6);
    sleep_ms(100);
    audio_beep(&audio_cfg, 1000, 20, -6);

    printf("Device Initalized!\n");

    multicore_reset_core1();
    multicore_launch_core1(core_1);

    core_0();

}