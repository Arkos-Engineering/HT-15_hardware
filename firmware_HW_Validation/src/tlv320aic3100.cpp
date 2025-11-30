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
    //Reset the audio amplifier
    //wait at least 1ms after calling this function to allow reset to complete
    this->write_page(0); //set to page 0
    this->write_register(0x01, 0x01); //write reset command to reset register

} // Reset the audio amplifier

void TLV320AIC3100::init(i2c_inst_t *i2c_bus, int i2c_address){
    //Initialize TLV320AIC3100 Audio Amplifier
    //be sure to initialize I2C bus before calling this function
    //ensure audio amp power is enabled before calling this function

    this->control_bus = i2c_bus;
    this->control_address = i2c_address;

    //reset audio amp
    this->reset();
    sleep_ms(1);

}

void TLV320AIC3100::reinit(){
    //Re-initialize the audio amplifier after power cycle
    this->init(this->control_bus, this->control_address);
}