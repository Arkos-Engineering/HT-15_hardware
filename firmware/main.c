#include <stdio.h>
#include <stdint.h>
#include <pico.h>
#include "pico/stdlib.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef u32 time32;

typedef float f32;
typedef double f64;

typedef _Bool bool8;

typedef char const * c_str;

#define U32_MAX UINT32_MAX

#define IFOR(i, v) for(u32 i = 0; i < v; ++i)
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))
#define alignof(x) __alignof__(x)

#define PINS \
    X(btn_mtx_a,           0)\
    X(btn_mtx_b,           1)\
    X(btn_mtx_c,           2)\
    X(btn_mtx_d,           3)\
    X(btn_mtx_0,           4)\
    X(btn_mtx_1,           5)\
    X(btn_mtx_2,           6)\
    X(btn_mtx_3,           7)\
    X(btn_mtx_4,           8)\
    X(btn_mtx_5,           9)\
    X(spi1_clk,            10)\
    X(spi1_sdi,            11)\
    X(spi1_sdo,            12)\
    X(audioamp_masterclk,  13)\
    X(audioamp_sdi,        14)\
    X(audioamp_scl,        15)\
    X(audioamp_wordselect, 16)\
    X(audioamp_sdo,        17)\
    X(sd_cs,               18)\
    X(sd_card_detect,      19)\
    X(flash_cs,            20)\
    X(display_cs,          21)\
    X(display_busy,        22)\
    X(display_reset,       23)\
    X(chgr_stat,           24)\
    X(audioamp_power,      25)\
    X(mars,                26)\
    X(led_status,          27)\
    X(mic_scl,             28)\
    X(mic_sdo,             29)\
    X(mic_wordselect,      30)\
    X(btn_enc_a,           31)\
    X(btn_enc_b,           32)\
    X(rf_gpio2,            33)\
    X(rf_gpio1,            34)\
    X(rf_gpio0,            35)\
    X(rf_sdo,              36)\
    X(rf_sel,              37)\
    X(rf_sclk,             38)\
    X(rf_sdi,              39)\
    X(rf_gpio3,            40)\
    X(headset_ptt,         41)\
    X(i2c1_sda,            42)\
    X(i2c1_scl,            43)\
    X(headset_conn,        44)\
    X(pot_volume,          45)\
    X(chgr_conn,           46)\
    X(v_bat,               47)

typedef enum {
    #define X(name, pin) name = pin,
    PINS
    #undef X
    pin_max_enum,
    pin_unknown = U32_MAX
} pin;

#define KEYS\
    X(up)\
    X(down)\
    X(left)\
    X(right)\
    X(lock)\
    X(side1)\
    X(side2)\
    X(ptt)\
    X(back)\
    X(enter)\
    X(1)\
    X(2)\
    X(3)\
    X(4)\
    X(5)\
    X(6)\
    X(7)\
    X(8)\
    X(9)\
    X(0)\
    X(star)\
    X(hash)

typedef enum {
    #define X(key) key_##key,
    KEYS
    #undef X
    key_max_enum,
    key_unknown = UINT32_MAX
} key_tag;
typedef struct key{u8 value;}key;

const u8 button_sense_pin[] = {btn_mtx_0, btn_mtx_1, btn_mtx_2, btn_mtx_3, btn_mtx_4, btn_mtx_5};
const u8 button_power_pin[] = {btn_mtx_a, btn_mtx_b, btn_mtx_c, btn_mtx_d};

#define KEY_STATES\
    X(released)\
    X(pressed)\
    X(repeat)

typedef enum{
    #define X(s) key_state_##s,
    KEY_STATES
    #undef X
} key_state;

key_state key_states[key_max_enum] = {key_state_released};
u32 last_key_changed_timestamp[key_max_enum] = {0};
u32 millis_until_repeating = 420;

#define BUTTONS_SIZE ARRAY_SIZE(button_power_pin) * ARRAY_SIZE(button_sense_pin)
#define BUTTON_DEBOUNCE_BUFFER_SIZE 1<<2
bool8 button_debounce_buffer[BUTTONS_SIZE * BUTTON_DEBOUNCE_BUFFER_SIZE] = {0}; 
u32 button_debounce_buffer_index = 0;

key_tag key_map[BUTTONS_SIZE] = {
    key_left, key_up,   key_right, key_side2,
    key_back, key_down, key_enter, key_side1,
    key_1,    key_2,    key_3,     key_ptt,
    key_4,    key_5,    key_6,     key_lock,
    key_7,    key_8,    key_9,     key_unknown,
    key_star, key_0,    key_hash,  key_unknown,
};

/* alignment represents the size of the buffer at 1 << alignment kindof like int2 int3 in fasm */
static inline u32 circle_buffer_index_at(u8 alignment, i32 index){
        return index & ((1<<alignment)-1);
}

/* returns time in milis sense device turned on */
time32 get_time(){
    /* TODO:*/
    return 0;
}

/* check physical user input. like buttons */
void poll_input(){
    /* */
    u32 columns = ARRAY_SIZE(button_power_pin);
    u32 rows = ARRAY_SIZE(button_sense_pin);
    IFOR(c, columns){
        gpio_put(button_power_pin[c], 1);
        sleep_us(1);
        IFOR(r, rows){
            bool8 pin = gpio_get(button_sense_pin[r]);
            button_debounce_buffer[(button_debounce_buffer_index * columns * rows) + c * rows + r] = pin;
        }
        gpio_put(button_power_pin[c], 0);
        sleep_us(1);
    }
    /* evaluate if a button is released or not */
    bool8 buttons[BUTTONS_SIZE] = {0};
    IFOR(b, BUTTONS_SIZE) {
        IFOR(d, BUTTON_DEBOUNCE_BUFFER_SIZE){
            u32 buffer_offset = circle_buffer_index_at(2, button_debounce_buffer_index - d);
            /* quick and durty but if the last 4 loops and the button is off we can assume its actually off and not ringing.*/
            buttons[b] |= button_debounce_buffer[buffer_offset * BUTTONS_SIZE + b]; 
        }
    }
    button_debounce_buffer_index = circle_buffer_index_at(2, button_debounce_buffer_index+1);
    
    IFOR(b, BUTTONS_SIZE){
        u32 key = key_map[b];

        key_state new_key_state = key_state_released; 
        if(buttons[b]){
           new_key_state = key_state_pressed;
        }
        if(key_states[key] == key_state_pressed && new_key_state == key_state_pressed){
            time32 time_sense_last_change = last_key_changed_timestamp[key] - get_time();
            if(time_sense_last_change > millis_until_repeating){
                key_states[key] = key_state_repeat;
            }
        }
    }
}

void print_button_debounce_buffer(){
    IFOR(i, 1<<2){
        printf("button buffer %u:\n", i);
        IFOR(b, BUTTONS_SIZE){
            printf("button %u state %u", b, button_debounce_buffer[(i * BUTTONS_SIZE) + b]);
        }
        printf("\n");
    }
}

void print_key_states(){
    static c_str key_names[] = {
        #define X(name) #name
        KEYS
        #undef X
    };

    static c_str key_state_names[] = {
        #define X(name) #name
        KEY_STATES
        #undef X
    };

    IFOR(k, key_max_enum) printf("key %s : %s \n", key_names[k], key_state_names[key_states[k]]);
}

int main(){
        /* initalize_daughter_board();
        initalize_configuration();
        initialze_radio_stored_state(); */

        puts("blalalalalal");
        IFOR(i, 6){
                gpio_init(button_sense_pin[i]);
                gpio_set_dir(button_sense_pin[i], 0);
                gpio_pull_down(button_sense_pin[i]);
        }
        IFOR(i, 4){
                gpio_init(button_power_pin[i]);
                gpio_set_dir(button_power_pin[i], 0);
                gpio_pull_down(button_power_pin[i]);
        }

        /* main loop */
        for(;;){
            poll_input();
            print_button_debounce_buffer();
            print_key_states();
            /*TODO@Zea as of December 20 2025: make this try to keep the same interval between loops*/
            sleep_ms(14);
        }
}

