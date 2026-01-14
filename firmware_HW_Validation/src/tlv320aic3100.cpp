// tlv320aic3100.cpp
#include "tlv320aic3100.h"


bool TLV320AIC3100::write_page(uint8_t page){
    //Set register page
    this->current_page = page;
    return this->write_register(0x00, page);
}

bool TLV320AIC3100::write_register(uint8_t reg_address, uint8_t value){
    //Write value to register
    uint8_t tx_data[] = {reg_address, value};
    return (i2c_write_blocking(this->control_bus, this->control_address, tx_data, 2, false)>0);
}

uint8_t TLV320AIC3100::read_register(uint8_t reg_address){
    uint8_t rx_data = 0;

    //read value from register
    i2c_write_blocking(this->control_bus, this->control_address, &reg_address, 1, true);
    i2c_read_blocking(this->control_bus, this->control_address, &rx_data, 1, false);
    return rx_data;
}

void TLV320AIC3100::reset(){
    //Reset the audio amplifier using the hardware reset pin
    gpio_put(this->reset_pin, 0); //start_reset
    sleep_us(1);
    gpio_put(this->reset_pin, 1); //finish_reset
    sleep_us(1);
}

void TLV320AIC3100::reset_soft(){
    //Reset the audio amplifier through I2C command
    //wait at least 1ms after calling this function to allow reset to complete
    this->write_page(0); //set to page 0
    this->write_register(0x01, 0x01); //write reset command to reset register
    sleep_us(1);
}

void TLV320AIC3100::init(i2c_inst_t *i2c_bus, int i2c_address, uint8_t reset_gpio_pin, uint8_t master_clock_pin){
    //Initialize TLV320AIC3100 Audio Amplifier
    //be sure to initialize I2C bus before calling this function
    //ensure audio amp power is enabled before calling this function

    this->control_bus = i2c_bus;
    this->control_address = i2c_address;
    this->reset_pin = reset_gpio_pin;
    this->mclk_pin = master_clock_pin;

    this->reinit();

}

void TLV320AIC3100::start_system_clock(){
    //configure microcontroller to output required MCLK signal on pin specified by master_clock_pin, derived from system clock

    //set clkout to the crystal oscillator frequency (12MHz)
    clock_gpio_init(this->mclk_pin, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 1);
}

void TLV320AIC3100::stop_system_clock(){
    //disable MCLK output
    gpio_init(this->mclk_pin);              // makes it a normal GPIO again
    gpio_set_dir(this->mclk_pin, GPIO_OUT);
    gpio_put(this->mclk_pin, 0);
}

void TLV320AIC3100::reinit(){

    //setup reset pin
    gpio_init(this->reset_pin);
    gpio_set_dir(this->reset_pin, GPIO_OUT);

    this->start_system_clock(); //initialize system clock output
    
    //reset audio amp
    this->reset();

    this->set_dac_processing_block(25); //enable all things

    //below sets dac and adc clock dividers to 16kHz sample rate with 12MHz MCLK
    this->configure_clock_div(12, 125, 3, 2, 128, 3, 2);

    this->set_dac_power(true, true); //power on both DAC channels
    this->set_dac_mute(false, false); //unmute audio output
    this->set_volume(128); //set unity gain volume
    this->set_speaker_driver_enable(true); //power on speaker amplifier

    // //unmute headphone amp
    // this->write_page(1); //set to page 1
    // this->write_register(42, 0b00000100); //unmute HRL
    // this->write_register(43, 0b00000100); //unmute HPR

}

void TLV320AIC3100::configure_clock_div(uint8_t timer, uint16_t dosr, uint8_t ndac, uint8_t mdac, uint8_t aosr, uint8_t nadc, uint8_t madc){
    //TODO Ben Pratt 12-20-2025, I need to configure the PLL because the ADC cannot run directly at 16kHz from MCLK due to aosr requirements. closest is 15.625kHz
    //Configure clock dividers
    //*dac or *adc range 0-127, writing 0 to any of the dividers will set them to 128
    //dosr range 0-1023, writing 0 will set to 1024
    //aosr can only be 32, 64, 128, 0, writing 0 will set to 256

	/* 16k rate, 12MHz MCLK */
    //mclk dosr ndac mdac aosr nadc madc
	//{12000000,125,3,2,128,3,2}, //16kHz dac, 15.625kHz adc

    //set timer clock divider, This should be set so that MCLK / timer ~= 1MHz
    this->write_page(3); //set to page 3
    this->write_register(16, (timer & 0x0b0111111) | 0b10000000); //set timer clock divider

    this->write_page(0); //set to page 0
    this->write_register(11, (ndac & 0x0b0111111) | 0b10000000); //set ndac clock divider
    this->write_register(12, (mdac & 0x0b0111111) | 0b10000000); //set mdac clock divider
    this->write_register(13, (uint8_t)((dosr >> 8) & 0b00000011)); //set dosr clock divider upper byte
    this->write_register(14, (uint8_t) dosr & 0xFF); //set dosr clock divider lower byte


    this->write_register(18, (ndac & 0x0b0111111) | 0b10000000); //set ndac clock divider
    this->write_register(19, (mdac & 0x0b0111111) | 0b10000000); //set mdac clock divider
    
    // aosr only accepts specific values
    if (aosr > 128){
        this->write_register(20, 0); //set aosr clock divider to 256
    } else if (aosr > 64){
        this->write_register(20, 128); //set aosr clock divider to 128
    } else if (aosr > 32){
        this->write_register(20, 64); //set aosr clock divider to 64
    } else {
        this->write_register(20, 32); //set aosr clock divider to 32
    }
}


void TLV320AIC3100::set_dac_processing_block(uint8_t block){
    //Set DAC processing block
    this->write_page(0); //set to page 0
    this->write_register(60, block); //set processing block
}

void TLV320AIC3100::set_dac_mute(bool mute_l, bool mute_r){
    //Mute or unmute audio output
    this->write_page(0); //set to page 0
    this->write_register(64, 0b00000010 | (mute_l ? 0b1000 : 0x00) | (mute_r ? 0b0100 : 0x00)); //set mute registers
}

void TLV320AIC3100::set_dac_power(bool pow_l, bool pow_r){
    //Set DAC power state
    this->write_page(0); //set to page 0
    this->write_register(63, 0b00010101 | (pow_l ? 0b10000000 : 0) | (pow_r ? 0b01000000 : 0b0)); //set DAC power registers
}

void TLV320AIC3100::set_speaker_driver_enable(bool enable){
    //Set overall amplifier power state
    this->write_page(1); //set to page 0
    this->write_register(32, enable ? 0b10000110 : 0b00000110); //set speaker amplifier power register
    this->write_register(42, enable ? 0b00000101 : 0b00000001); //set speaker amplifier mute
}


void TLV320AIC3100::set_volume(uint8_t volume){
    //sets amp output volume, accepts 1 to 176. 128 is unity gain. each step is 0.5dB. Numbers outside this range will be clamped.

    int8_t reg_value = volume - 128;
    if (reg_value > 48) reg_value = 48; //max volume is +24dB, 48 steps of 0.5dB
    if (reg_value = -128) reg_value = -127; //min volume is -63.5dB, -127 steps of 0.5dB

    this->write_page(0); //set to page 0
    this->write_register(65, reg_value); //left headphone out volume
    this->write_register(66, reg_value); //right headphone out volume
}

void TLV320AIC3100::beep(uint8_t volume){
    //Generate a beep sound at specified volume (0-100)
    if(volume>100) volume = 100;

    //map volume 0-100 to register value 0x00-0x1F
    uint8_t reg_volume = ((uint8_t)((float)(100-volume)*0.64)) & 0b00111111;

    this->write_page(0); //set to page 0
    this->write_register(72, 0b10000010); //set right channel beep volume to follow left channel
    this->write_register(71, reg_volume | 0b10000000); //set left channel beep volume and start beep
    // this->write_register(71, 0x10000000); //set left channel beep volume and start beep
}