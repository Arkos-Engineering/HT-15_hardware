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

#include "pindefs.cpp"
#include "keypad.h"
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
tlv320dac3100_t audio_amp;


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

void audioamp_reset_hard(){
    gpio_put(AUDIOAMP_RESET, 0); //start_reset
    sleep_us(1);
    gpio_put(AUDIOAMP_RESET, 1); //finish_reset
    sleep_ms(2);
}

void audioamp_start_system_clock(){
    //configure microcontroller to output required MCLK signal on pin specified by master_clock_pin, derived from system clock

    //set clkout to the crystal oscillator frequency (12MHz)
    clock_gpio_init(AUDIOAMP_MASTERCLK, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 1);
}

void audioamp_stop_system_clock(){
    //disable MCLK output
    gpio_init(AUDIOAMP_MASTERCLK);              // makes it a normal GPIO again
    gpio_set_dir(AUDIOAMP_MASTERCLK, GPIO_OUT);
    gpio_put(AUDIOAMP_MASTERCLK, 0);
}

void init_audio_amp(){
    // audio_amp.init(i2c1, ADDRESS_I2C_AUDIOAMP, AUDIOAMP_RESET, AUDIOAMP_MASTERCLK); //initialize audio amp
    audioamp_start_system_clock();
    
    // Setup reset pin and perform hard reset BEFORE initializing
    gpio_set_dir(AUDIOAMP_RESET, GPIO_OUT);
    audioamp_reset_hard();
    
    tlv320_init(&audio_amp, i2c1, ADDRESS_I2C_AUDIOAMP);

    // Set DAC processing block (PRB_P25 supports beep generator)
    tlv320_set_dac_processing_block(&audio_amp, 25);

    // Set clock dividers for 16kHz sample rate
    // MCLK=12MHz, NDAC=3, MDAC=2, DOSR=125
    // DAC_CLK = MCLK / NDAC = 12MHz / 3 = 4MHz
    // DAC_MOD_CLK = DAC_CLK / MDAC = 4MHz / 2 = 2MHz  
    // DAC_FS = DAC_MOD_CLK / DOSR = 2MHz / 125 = 16kHz
    tlv320_set_ndac(&audio_amp, true, 3);
    tlv320_set_mdac(&audio_amp, true, 2);
    tlv320_set_dosr(&audio_amp, 125);

    // Configure codec interface - I2S, 16-bit, codec is master (bclk_out=true, wclk_out=true)
    // IMPORTANT: Must be set BEFORE configuring BCLK dividers per datasheet power-up sequence
    tlv320_set_codec_interface(&audio_amp, TLV320DAC3100_FORMAT_I2S, TLV320DAC3100_DATA_LEN_16, true, true);
    
    // Set codec clock input to MCLK (12MHz from RP2350)
    tlv320_set_codec_clock_input(&audio_amp, TLV320DAC3100_CODEC_CLKIN_MCLK);

    // Configure BCLK generation:
    // For 16kHz sample rate with 16-bit stereo I2S: BCLK = 16kHz * 16 * 2 = 512kHz
    // Using DAC_MOD_CLK (2MHz) as source: BCLK_N = 2MHz / 512kHz = 4 (gives 500kHz, acceptable)
    // Per TLV320DAC3100 datasheet Section 7.4.2, when codec is master BCLK must be properly derived
    tlv320_set_bclk_n(&audio_amp, true, 4);  // Enable BCLK divider with N=4
    
    // Configure BCLK output behavior (Page 1, Reg 0x1D):
    // - invert_bclk=false: normal polarity
    // - active_when_powered_down=true: keep BCLK running 
    // - source=DAC_MOD_CLK: derive from DAC_MOD_CLK (2MHz) for cleaner division
    tlv320_set_bclk_config(&audio_amp, false, true, TLV320DAC3100_BCLK_SRC_DAC_MOD_CLK);

    // Configure timer/delay clock divider (Page 3, Reg 16)
    // For 12MHz MCLK, divider of 12 gives ~1MHz timer clock for beep generator
    // Per TLV320DAC3100 datasheet Section 7.4.3
    tlv320_config_delay_divider(&audio_amp, true, 12);

    // Enable DAC data path - both channels on, normal routing
    tlv320_set_dac_data_path(&audio_amp, true, true, TLV320_DAC_PATH_NORMAL, TLV320_DAC_PATH_NORMAL, TLV320_VOLUME_STEP_2SAMPLE);

    // Route DAC to speaker mixer (not headphone)
    tlv320_configure_analog_inputs(&audio_amp, TLV320_DAC_ROUTE_MIXER, TLV320_DAC_ROUTE_MIXER, false, false, false, false);

    // Unmute DAC channels
    tlv320_set_dac_volume_control(&audio_amp, true, true, TLV320_VOL_RIGHT_TO_LEFT);
    
    // Set DAC digital volume (0dB for left and right channels)
    tlv320_set_channel_volume(&audio_amp, false, 0);  // Left channel 0dB
    tlv320_set_channel_volume(&audio_amp, true, 0);   // Right channel 0dB

    // Enable speaker amplifier
    tlv320_enable_speaker(&audio_amp, true);
    
    // Configure speaker driver - gain and unmute
    // TLV320_SPK_GAIN_6DB = +6dB gain stage
    tlv320_configure_spk_pga(&audio_amp, TLV320_SPK_GAIN_6DB, true);
    
    // Set speaker analog volume - route enabled, 0dB gain
    tlv320_set_spk_volume(&audio_amp, true, 0);
    
    // Wait for output drivers to stabilize
    sleep_ms(50);
    
    // Debug: Verify clock configuration
    printf("=== TLV320 Clock Configuration Debug ===\n");
    
    // Read back NDAC (Page 0, Reg 0x0B)
    uint8_t ndac_reg = tlv320_read_register(&audio_amp, 0, TLV320DAC3100_REG_NDAC);
    printf("NDAC (P0R11): 0x%02X (enabled=%d, val=%d)\n", ndac_reg, (ndac_reg >> 7) & 1, ndac_reg & 0x7F);
    
    // Read back MDAC (Page 0, Reg 0x0C)
    uint8_t mdac_reg = tlv320_read_register(&audio_amp, 0, TLV320DAC3100_REG_MDAC);
    printf("MDAC (P0R12): 0x%02X (enabled=%d, val=%d)\n", mdac_reg, (mdac_reg >> 7) & 1, mdac_reg & 0x7F);
    
    // Read back BCLK_N (Page 0, Reg 0x1E)
    uint8_t bclk_n_reg = tlv320_read_register(&audio_amp, 0, TLV320DAC3100_REG_BCLK_N);
    printf("BCLK_N (P0R30): 0x%02X (enabled=%d, val=%d)\n", bclk_n_reg, (bclk_n_reg >> 7) & 1, bclk_n_reg & 0x7F);
    
    // Read back Codec Interface (Page 0, Reg 0x1B)
    uint8_t codec_if = tlv320_read_register(&audio_amp, 0, TLV320DAC3100_REG_CODEC_IF_CTRL1);
    printf("CODEC_IF (P0R27): 0x%02X (bclk_out=%d, wclk_out=%d)\n", codec_if, (codec_if >> 3) & 1, (codec_if >> 2) & 1);
    
    // Read back BCLK_CTRL2 (Page 1, Reg 0x1D) - critical for BCLK source
    uint8_t bclk_ctrl2 = tlv320_read_register(&audio_amp, 1, TLV320DAC3100_REG_BCLK_CTRL2);
    printf("BCLK_CTRL2 (P1R29): 0x%02X (src=%d, active_pd=%d)\n", bclk_ctrl2, bclk_ctrl2 & 0x03, (bclk_ctrl2 >> 2) & 1);
    
    printf("=== End Clock Debug ===\n");
    printf("Audio amp initialized\n");
}

/// @brief Generate a beep tone on the speaker
/// @param frequency_hz Frequency of the beep in Hz (recommended 500-2000Hz)
/// @param duration_ms Duration of the beep in milliseconds
/// @param volume_db Volume in dB (0 = max, -61 = min)
void audioamp_beep(uint16_t frequency_hz, uint16_t duration_ms, int8_t volume_db){
    // Sample rate is 16kHz based on our clock divider configuration
    const uint32_t sample_rate = 16000;
    
    // Frequency must be less than sample_rate/4 per datasheet
    if (frequency_hz >= sample_rate / 4) {
        frequency_hz = sample_rate / 4 - 1;
    }
    
    // Configure the beep tone (sin/cos coefficients and length)
    // This calculates the proper phase increment for the DDS
    if (!tlv320_configure_beep_tone(&audio_amp, (float)frequency_hz, duration_ms, sample_rate)) {
        printf("Failed to configure beep tone\n");
        return;
    }
    
    // Set beep volume (0dB max, -61dB min)
    // Using same volume for both channels
    tlv320_set_beep_volume(&audio_amp, volume_db, volume_db);
    
    // Start the beep
    tlv320_enable_beep(&audio_amp, true);
    
    printf("Beep: %dHz, %dms, %ddB\n", frequency_hz, duration_ms, volume_db);
}

void init_all(){
    //initialize all necessary pins

    //init I2C1
    I2C1_init();

    // audio_amp.init(i2c1, ADDRESS_I2C_AUDIOAMP, AUDIOAMP_RESET, AUDIOAMP_MASTERCLK); //initialize audio amp
    init_audio_amp();

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

        if (!(counter%2000)){
            I2C1_scan_bus();
            
            // Debug: Read speaker driver register to verify configuration
            uint8_t spk_driver = tlv320_read_register(&audio_amp, 1, TLV320DAC3100_REG_SPK_DRIVER);
            printf("SPK_DRIVER (P1R42): 0x%02X\n", spk_driver);
            
            // Read DAC flags to verify power state
            bool left_dac_on, right_dac_on, left_classd_on, right_classd_on;
            tlv320_get_dac_flags(&audio_amp, &left_dac_on, NULL, &left_classd_on, 
                                 &right_dac_on, NULL, &right_classd_on, NULL, NULL);
            printf("DAC: L=%d R=%d, ClassD: L=%d R=%d\n", 
                   left_dac_on, right_dac_on, left_classd_on, right_classd_on);

            printf(tlv320_is_speaker_shorted(&audio_amp) ? "Speaker short detected!\n" : "No speaker short.\n");
            
            // Generate a 1kHz beep for 200ms at 0dB (max volume)
            audioamp_beep(1000, 200, 0);
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