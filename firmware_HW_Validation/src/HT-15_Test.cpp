#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <vector>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"

#include "pindefs.cpp"
#include "keypad.h"
#include "tlv320aic3100.h"

//global variables
#define ADDRESS_I2C_AUDIOAMP 0b0011000

//key names for printing
char key_names[23][6] = {
    "Null", "Up", "Down", "Left", "Right", "Lock", "Side1", "Side2",
    "PTT", "Back", "Enter", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "*", "#"
};

//Audio Amplifier object
TLV320AIC3100 audio_amp;


/// I2C1 Initialization
void I2C1_init(){
    // Force-reset the I2C1 peripheral in case the Wi-Fi core or bootloader locked it
    i2c1_hw->enable = 0;
    sleep_us(10);

    // Initialize I2C1 at 100kHz
    i2c_init(i2c1, 100000);

    // Set up GPIO pins for I2C1
    gpio_set_function(I2C1_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL, GPIO_FUNC_I2C);
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

void init_all(){
    //initialize all necessary pins

    //init I2C1
    I2C1_init();


    audio_amp.init(i2c1, ADDRESS_I2C_AUDIOAMP, AUDIOAMP_RESET, AUDIOAMP_MASTERCLK); //initialize audio amp

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
        if (!(counter%1000)){
            current_volume = get_volume_pot();
            if(abs(current_volume-last_volume)>2){
                last_volume = current_volume;
                printf("Volume: %d\n", current_volume);
            }
        }       

        if (counter%2000==1000){
            // I2C1_scan_bus();
            // audio_amp.write_page(0);
            // printf("Amp_PB: 0x%02X\n", audio_amp.read_register(60));
            // audio_amp.set_volume(current_volume);
            audio_amp.beep(100);
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
    init_all();
    sleep_ms(5000);

    printf("Device Initalized!\n");

    multicore_reset_core1();
    multicore_launch_core1(core_1);

    core_0();

}