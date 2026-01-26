// keypad.h
// Public API for keypad scanning extracted from HT-15_Test.cpp
// Provides initialization and periodic scan functions for the 6x4 button matrix and encoder/pot reads.

#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <vector>

#include "pico/stdlib.h"

#include "pindefs.h"

enum Keys{
    KEY_NULL,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_LOCK,
    KEY_SIDE1,
    KEY_SIDE2,
    KEY_PTT,
    KEY_BACK,
    KEY_ENTER,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_STAR,
    KEY_HASH
};

class Keypad{
    public:
        // Initialize keypad GPIOs and ADC inputs used by keypad code.
        static void init();

        // Run one iteration of keypad processing. This will scan the matrix, handle debouncing,
        // Call this periodically from the main loop.
        static void keypad_poll();

        //returns array of buttons pressed since last poll. Maximum of 5 buttons can be returned. Buttons pressed after this buffer fills up will be ignored.
        static std::vector<Keys> get_buttons_pressed();

    private:
        static uint8_t button_sense_pin[6];
        static uint8_t button_pwr_pin[4];
        static Keys button_locations[6][4];
        static char button_names[6][4][6];

        static bool button_debounce_states[6][4][2];
        static bool button_pressed_last_loop[6][4];
        static bool button_debounce_pointer;

        static std::vector<Keys> buttons_pressed_since_last_poll;

};

#endif // KEYPAD_H
