// keypad.c
#include "keypad.h"

    uint8_t Keypad::button_sense_pin[6] = {BTN_MTX_0, BTN_MTX_1, BTN_MTX_2, BTN_MTX_3, BTN_MTX_4, BTN_MTX_5};
    uint8_t Keypad::button_pwr_pin[4] = {BTN_MTX_A, BTN_MTX_B, BTN_MTX_C, BTN_MTX_D};
    Keys Keypad::button_locations[6][4] = {
        {KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_SIDE2},
        {KEY_BACK, KEY_DOWN, KEY_ENTER, KEY_SIDE1},
        {KEY_1, KEY_2, KEY_3, KEY_PTT},
        {KEY_4, KEY_5, KEY_6, KEY_LOCK},
        {KEY_7, KEY_8, KEY_9, KEY_NULL},
        {KEY_STAR, KEY_0, KEY_HASH, KEY_NULL}
    };

    bool Keypad::button_debounce_states[6][4][2] = {0};
    bool Keypad::button_pressed_last_loop[6][4] = {0};
    bool Keypad::button_debounce_pointer = 0;

    std::vector<Keys> Keypad::buttons_pressed_since_last_poll = {};

void Keypad::init(){
    // init button pins
    for (int i = 0; i < 6; i++) {
        gpio_init(Keypad::button_sense_pin[i]);
        gpio_set_dir(Keypad::button_sense_pin[i], GPIO_IN);
        gpio_pull_down(Keypad::button_sense_pin[i]);
    }
    for (int i = 0; i < 4; i++) {
        gpio_init(Keypad::button_pwr_pin[i]);
        gpio_set_dir(Keypad::button_pwr_pin[i], GPIO_OUT);
        gpio_put(Keypad::button_pwr_pin[i], 0);
    }
}

void Keypad::keypad_poll(){
    // Scan the keypad matrix and handle debouncing. Should be called every 10ms
    for (int j = 0; j < 4; j++) {
        gpio_put(Keypad::button_pwr_pin[j], 1);
        sleep_us(1);
        for (int i = 0; i < 6; i++) {
            Keypad::button_debounce_states[i][j][Keypad::button_debounce_pointer] = gpio_get(Keypad::button_sense_pin[i]);
        }
        gpio_put(Keypad::button_pwr_pin[j], 0);
        sleep_us(1);
    }
    Keypad::button_debounce_pointer = !Keypad::button_debounce_pointer;
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 4; j++) {
            if(Keypad::button_debounce_states[i][j][0] && Keypad::button_debounce_states[i][j][1]){
                if(!Keypad::button_pressed_last_loop[i][j]){
                    Keypad::buttons_pressed_since_last_poll.push_back(Keypad::button_locations[i][j]);
                }
                Keypad::button_pressed_last_loop[i][j] = true;
            } else{
                Keypad::button_pressed_last_loop[i][j] = false;
            }
        }
    }
}

std::vector<Keys> Keypad::get_buttons_pressed(){
    // Return the accumulated buttons and clear the stored list efficiently.
    // Use move semantics (swap with a temporary) so we avoid copying the vector.
    std::vector<Keys> result;
    result.swap(Keypad::buttons_pressed_since_last_poll);
    // buttons_pressed_since_last_poll is now empty; return the former contents.
    return result;
}