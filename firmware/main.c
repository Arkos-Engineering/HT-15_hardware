#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <pico.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include <pico/bootrom.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef u32 time32;

/* TODO @Zea as of December 20 2025: we should probably use fixed point values here instead of software floats
    At some point we should poison these values and remove all floats and doubles from the code. */
typedef float f32;
typedef double f64;

typedef _Bool bool8;

typedef char const * c_str;

#define U32_MAX UINT32_MAX

#define ifor(i, v) for(u32 i = 0; i < (u32)v; ++i)
#define array_size(a) (sizeof(a)/sizeof(*a))
#define alignof(x) __alignof__(x)

#define PINS \
    X(buttonmatrix_a,       0)\
    X(buttonmatrix_b,       1)\
    X(buttonmatrix_c,       2)\
    X(buttonmatrix_d,       3)\
    X(buttonmatrix_0,       4)\
    X(buttonmatrix_1,       5)\
    X(buttonmatrix_2,       6)\
    X(buttonmatrix_3,       7)\
    X(buttonmatrix_4,       8)\
    X(buttonmatrix_5,       9)\
    X(spi1_clk,             10)\
    X(spi1_sdi,             11)\
    X(spi1_sdo,             12)\
    X(audioamp_masterclock, 13)\
    X(audioamp_sdi,         14)\
    X(audioamp_scl,         15)\
    X(audioamp_wordselect,  16)\
    X(audioamp_sdo,         17)\
    X(sd_cs,                18)\
    X(sd_card_detect,       19)\
    X(flash_cs,             20)\
    X(display_cs,           21)\
    X(display_busy,         22)\
    X(display_reset,        23)\
    X(charger_status,       24)\
    X(audioamp_power,       25)\
    X(mars,                 26)\
    X(led_status,           27)\
    X(mic_scl,              28)\
    X(mic_sdo,              29)\
    X(mic_wordselect,       30)\
    X(encoder_a,            31)\
    X(encoder_b,            32)\
    X(rf_gpio2,             33)\
    X(rf_gpio1,             34)\
    X(rf_gpio0,             35)\
    X(rf_sdo,               36)\
    X(rf_sel,               37)\
    X(rf_sclk,              38)\
    X(rf_sdi,               39)\
    X(rf_gpio3,             40)\
    X(headset_ptt,          41)\
    X(i2c1_sda,             42)\
    X(i2c1_scl,             43)\
    X(headset_conn,         44)\
    X(pot_volume,           45)\
    X(chgr_conn,            46)\
    X(v_bat,                47)

typedef enum {
    #define X(name, pin) pin_##name = pin,
    PINS
    #undef X
    pin_max_enum,
    pin_unknown = U32_MAX
} pin;

#define BLINK_CODES\
    X(okay, 0)\
    X(stdio_failed_to_initalize, 3)\
    X(next_blink_code_idk, 7)\

typedef enum{
    #define X(name, code) blink_code_##name = code,
    BLINK_CODES
    #undef X
    blink_codes_max_enum,
    blink_codes_unknown = U32_MAX
} blink_code;

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

const u8 button_sense_pin[] = {pin_buttonmatrix_0, pin_buttonmatrix_1, pin_buttonmatrix_2, pin_buttonmatrix_3, pin_buttonmatrix_4, pin_buttonmatrix_5};
const u8 button_power_pin[] = {pin_buttonmatrix_a, pin_buttonmatrix_b, pin_buttonmatrix_c, pin_buttonmatrix_d};

#define KEY_STATES\
    X(released)\
    X(pressed)\
    X(repeat)

typedef enum{
    #define X(s) key_state_##s,
    KEY_STATES
    #undef X
    key_state_max_enum,
    key_state_unknown = U32_MAX
} key_state;

key_state key_states[key_max_enum] = {key_state_released};
u32 last_key_changed_timestamp[key_max_enum] = {0};
u32 millis_until_repeating = 420;

#define BUTTONS_SIZE array_size(button_power_pin) * array_size(button_sense_pin)
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

/*  I2C1 Initialization */
void I2C1_init(){
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

typedef struct {
    i2c_inst_t *control_bus;     /* I2C bus instance */
    u8          control_address; /* 7 bit I2C address */
    u8          current_page;    /* current register page */
    u8          reset_pin;       /* GPIO pin for hardware reset */
    u8          volume;          /* volume for beep sound */
    u8          mclk_pin;        /* GPIO pin for master clock output from the rp235x */
} TLV320AIC3100;

static void  TLV320AIC3100_beep(TLV320AIC3100*amp, u8 volume);
static void  TLV320AIC3100_set_volume(TLV320AIC3100*amp, u8 volume);
static void  TLV320AIC3100_set_speaker_amplifier_power(TLV320AIC3100 * amp, bool8 power_on);
static void  TLV320AIC3100_set_dac_power(TLV320AIC3100*amp, bool8 pow_l, bool8 pow_r);
static void  TLV320AIC3100_set_mute(TLV320AIC3100*amp, bool8 mute_l, bool8 mute_r);
static void  TLV320AIC3100_reinit(TLV320AIC3100*amp);
static void  TLV320AIC3100_init_system_clock(TLV320AIC3100*amp);
static void  TLV320AIC3100_init(TLV320AIC3100*amp, i2c_inst_t *i2c_bus, int i2c_address, u8 reset_gpio_pin, u8 master_clock_pin);
static void  TLV320AIC3100_reset_soft(TLV320AIC3100*amp);
static void  TLV320AIC3100_reset(TLV320AIC3100*amp);
static u8    TLV320AIC3100_read_register(TLV320AIC3100*amp, u8 reg_address);
static bool8 TLV320AIC3100_write_register(TLV320AIC3100*amp, u8 reg_address, u8 value);
static bool8 TLV320AIC3100_write_page(TLV320AIC3100*amp, u8 page);

static bool8 TLV320AIC3100_write_page(TLV320AIC3100 *amp, u8 page){
    /* Set register page */
    amp->current_page = page;
    return TLV320AIC3100_write_register(amp, 0x00, page);
}

static bool8 TLV320AIC3100_write_register(TLV320AIC3100 * amp, u8 reg_address, u8 value){
    /* Write value to register */
    u8 tx_data[] = {reg_address, value};
    return (i2c_write_blocking(amp->control_bus, amp->control_address, tx_data, 2, false)>0);
}

static u8 TLV320AIC3100_read_register(TLV320AIC3100 * amp, u8 reg_address){
    u8 rx_data = 0;

    /* read value from register */
    i2c_write_blocking(amp->control_bus, amp->control_address, &reg_address, 1, true);
    i2c_read_blocking(amp->control_bus, amp->control_address, &rx_data, 1, false);
    return rx_data;
}

void TLV320AIC3100_reset(TLV320AIC3100 * amp){
    /* Reset the audio amplifier using the hardware reset pin */
    gpio_put(amp->reset_pin, 0); /* start_reset */
    sleep_us(1);
    gpio_put(amp->reset_pin, 1); /* finish_reset */
    sleep_us(1);
}

static void TLV320AIC3100_reset_soft(TLV320AIC3100 * amp){
    /* Reset the audio amplifier through I2C command */
    /* wait at least 1ms after calling amp function to allow reset to complete */
    TLV320AIC3100_write_page(amp, 0); /* set to page 0 */
    TLV320AIC3100_write_register(amp, 0x01, 0x01); /* write reset command to reset register */
    sleep_us(1);
}

static void TLV320AIC3100_init(TLV320AIC3100 * amp, i2c_inst_t *i2c_bus, int i2c_address, u8 reset_gpio_pin, u8 master_clock_pin){
    /* Initialize TLV320AIC3100 Audio Amplifier */
    /* be sure to initialize I2C bus before calling amp function */
    /* ensure audio amp power is enabled before calling amp function */

    amp->control_bus = i2c_bus;
    amp->control_address = i2c_address;
    amp->reset_pin = reset_gpio_pin;
    amp->mclk_pin = master_clock_pin;

    TLV320AIC3100_reinit(amp);

}

static void TLV320AIC3100_init_system_clock(TLV320AIC3100 * amp){
    /* configure microcontroller to output required MCLK signal on pin specified by master_clock_pin, derived from system clock */

    /* set clkout to the crystal oscillator frequency (12MHz) */
    /*  clock_gpio_init(amp->mclk_pin, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 1); */
}

static void TLV320AIC3100_reinit(TLV320AIC3100 * amp){

    /* setup reset pin */
    gpio_init(amp->reset_pin);
    gpio_set_dir(amp->reset_pin, GPIO_OUT);

    TLV320AIC3100_init_system_clock(amp); /* initialize system clock output */
    
    /* reset audio amp */
    TLV320AIC3100_reset(amp);


    TLV320AIC3100_set_dac_power(amp, true, true); /* power on both DAC channels */
    TLV320AIC3100_set_mute(amp, false, false); /* unmute audio output */
    TLV320AIC3100_set_volume(amp, 128); /* set unity gain volume */
    TLV320AIC3100_set_speaker_amplifier_power(amp, true); /* power on speaker amplifier */

    /* unmute speaker amp */
    TLV320AIC3100_write_page(amp, 1); /* set to page 1 */
    TLV320AIC3100_write_register(amp, 42, 0b00000100); /* unmute speaker */
}

static void TLV320AIC3100_set_mute(TLV320AIC3100 * amp, bool8 mute_l, bool8 mute_r){
    /* Mute or unmute audio output */
    TLV320AIC3100_write_page(amp, 0); /* set to page 0 */
    TLV320AIC3100_write_register(amp, 64, 0b00000010 | (mute_l ? 0b1000 : 0x00) | (mute_r ? 0b0100 : 0x00)); /* set mute registers */
}

static void TLV320AIC3100_set_dac_power(TLV320AIC3100 * amp, bool8 pow_l, bool8 pow_r){
    /* Set DAC power state */
    TLV320AIC3100_write_page(amp, 0); /* set to page 0 */
    TLV320AIC3100_write_register(amp, 63, 0b00010101 | (pow_l ? 0b10000000 : 0) | (pow_r ? 0b01000000 : 0b0)); /* set DAC power registers */
}

static void TLV320AIC3100_set_speaker_amplifier_power(TLV320AIC3100 * amp, bool8 power_on){
    /* Set overall amplifier power state */
    TLV320AIC3100_write_page(amp, 1); /* set to page 0 */
    TLV320AIC3100_write_register(amp, 32, power_on ? 0b10000110 : 0b00000110); /* set speaker amplifier power register */
}


static void TLV320AIC3100_set_volume(TLV320AIC3100 * amp, u8 volume){
    /* sets amp output volume, accepts 1 to 176. 128 is unity gain. each step is 0.5dB. Numbers outside amp range will be clamped. */

    int8_t reg_value = volume - 128;
    if (reg_value > 48) reg_value = 48; /* max volume is +24dB, 48 steps of 0.5dB */
    if (reg_value == -128) reg_value = -127; /* min volume is -63.5dB, -127 steps of 0.5dB */

    TLV320AIC3100_write_page(amp, 0); /* set to page 0 */
    TLV320AIC3100_write_register(amp, 65, reg_value); /* left headphone out volume */
    TLV320AIC3100_write_register(amp, 66, reg_value); /* right headphone out volume */
}

static void TLV320AIC3100_beep(TLV320AIC3100 * amp, u8 volume){
    /* Generate a beep sound at specified volume (0-100) */
    if(volume>100) volume = 100;

    /* map volume 0-100 to register value 0x00-0x1F */
    u8 reg_volume = ((u8)((float)(100-volume)*0.64)) & 0b00111111;

    TLV320AIC3100_write_page(amp, 0); /* set to page 0 */
    TLV320AIC3100_write_register(amp, 72, 0b00000011); /* set right channel beep volume to follow left channel */
    TLV320AIC3100_write_register(amp, 71, reg_volume | 0b10000000); /* set left channel beep volume and start beep */
}

/* returns time in milis sense device turned on */
time32 get_time(){
    /* TODO:*/
    return 0;
}

f32 get_battery_voltage(){
    adc_select_input(pin_v_bat-40); /* V_BAT is ADC7, ADC input is 0 indexed */
    return (f32)adc_read()*.003791; /* conversion factor for voltage divider and ADC step size (127/27)*(3.3/4095) */
}
u8 get_volume_pot(){
    adc_select_input(pin_pot_volume-40); /* POT_VOLUME is ADC5, ADC input is 0 indexed */
    return 99-((u8)((f32)adc_read()*0.02442));
}


/* TODO@Zea as of December 20 2025 add knobs */
void poll_input(){
    u32 columns = array_size(button_power_pin);
    u32 rows = array_size(button_sense_pin);
    ifor(c, columns){
        gpio_put(button_power_pin[c], 1);
        sleep_us(1);
        ifor(r, rows){
            bool8 pin = gpio_get(button_sense_pin[r]);
            button_debounce_buffer[(button_debounce_buffer_index * columns * rows) + r * columns + c] = pin;
        }
        gpio_put(button_power_pin[c], 0);
        sleep_us(1);
    }
    /* evaluate if a button is released or not */
    bool8 buttons[BUTTONS_SIZE] = {0};
    ifor(b, BUTTONS_SIZE) {
        ifor(d, BUTTON_DEBOUNCE_BUFFER_SIZE){
            u32 buffer_offset = circle_buffer_index_at(2, button_debounce_buffer_index - d);
            /* quick and durty but if the last 4 loops and the button is off we can assume its actually off and not ringing.*/
            buttons[b] = button_debounce_buffer[buffer_offset * BUTTONS_SIZE + b]; 
        }
    }
    button_debounce_buffer_index = circle_buffer_index_at(2, button_debounce_buffer_index+1);
    
    ifor(b, BUTTONS_SIZE){
        u32 key = key_map[b];
        if(key == key_unknown) continue;

        if(buttons[b]){
            if(key_states[key] == key_state_pressed){
                time32 time_sense_last_change = last_key_changed_timestamp[key] - get_time();
                if(time_sense_last_change > millis_until_repeating){
                    key_states[key] = key_state_repeat;
                }
            }else key_states[key] = key_state_pressed;
        }else{
            key_states[key] = key_state_released;
        }
    }

    /* process encoder */
    // encoder_state=((encoder_state<<1) | gpio_get(BTN_ENC_A)) & 0b111;/* store the last three states of the encoder A pin */
    // if(encoder_state==0b100){
    //     if(gpio_get(BTN_ENC_B)){
    //         printf("Channel +\n");
    //     } else{
    //         printf("Channel -\n");
    //     }
    //     encoder_state=0;
    // }

    /* read volume pot */
    // current_volume = get_volume_pot();
    // if(abs(current_volume-last_volume)>2){
    //     last_volume = current_volume;
    //     printf("Volume: %d\n", current_volume);
    // }
}

void print_button_debounce_buffer(){
    ifor(i, 1<<2){
        printf("button buffer %"PRIu32":\n", i);
        ifor(b, BUTTONS_SIZE){
            printf("button %"PRIu32" state %"PRIi8"\n", b, button_debounce_buffer[(i * BUTTONS_SIZE) + b]);
        }
        printf("\n");
    }
}

void print_key_states(){
    static c_str key_names[key_max_enum] = {
        #define X(name) #name,
        KEYS
        #undef X
    };

    static c_str key_state_names[key_max_enum] = {
        #define X(name) #name,
        KEY_STATES
        #undef X
    };

    ifor(k, key_max_enum){
        printf("key %s : %s \n", key_names[k], key_state_names[key_states[k]]);
    } 
}

void 


/*(../hardware/Datasheets/AES200200A00-1.54ENRS(1).pdf)*/

int main(){
    gpio_init(pin_led_status);
    gpio_set_dir(pin_led_status, GPIO_OUT);
    gpio_put(pin_led_status, 1);
    bool8 led_status_value = 1;

    if(!stdio_init_all()){

        ifor(i, blink_code_stdio_failed_to_initalize){
            sleep_ms(333);
            gpio_put(pin_led_status, 0);
            sleep_ms(333);
            gpio_put(pin_led_status, 1);
            /* reset_usb_boot(0,0); */
        }
    }

    ifor(i, 6){
            gpio_init(button_sense_pin[i]);
            gpio_set_dir(button_sense_pin[i], GPIO_IN);
            gpio_pull_down(button_sense_pin[i]);
    }
    ifor(i, 4){
            gpio_init(button_power_pin[i]);
            gpio_set_dir(button_power_pin[i], GPIO_OUT);
            gpio_pull_down(button_power_pin[i]);
    }

    gpio_init(pin_encoder_a);
    gpio_set_dir(pin_encoder_a, GPIO_IN);
    gpio_init(pin_encoder_a);
    gpio_set_dir(pin_encoder_a, GPIO_IN);

    adc_init();
    adc_gpio_init(pin_v_bat);
    adc_gpio_init(pin_pot_volume);

    /* TODO @Zea as of December 21 2025: do we need to do this here? */
    /* Force-reset the I2C1 peripheral in case the Wi-Fi core or bootloader locked it */
    i2c1_hw->enable = 0;
    sleep_us(10);
    i2c_init(i2c1, 100000);
    gpio_set_function(pin_i2c1_sda, GPIO_FUNC_I2C);
    gpio_set_function(pin_i2c1_scl, GPIO_FUNC_I2C);

    /* Initalize display spi1 for display, sd card ,external spi flash*/
    spi_init(spi1, 10000);
    gpio_set_function(pin_spi1_clk, GPIO_FUNC_SPI);
    gpio_set_function(pin_spi1_sdi, GPIO_FUNC_SPI);
    gpio_set_function(pin_spi1_sdo, GPIO_FUNC_SPI);
    gpio_init(pin_display_cs);
    gpio_init(pin_display_busy);
    gpio_init(pin_display_reset);
    gpio_set_dir(pin_display_cs, GPIO_OUT);
    gpio_set_dir(pin_display_busy, GPIO_IN);
    gpio_set_dir(pin_display_reset, GPIO_OUT);
    gpio_put(pin_display_cs, 0);
    gpio_put(pin_display_reset, 0);

    gpio_init(pin_sd_cs);
    gpio_set_dir(pin_sd_cs, GPIO_OUT);
    gpio_put(pin_sd_cs, 0);

    gpio_init(pin_flash_cs);
    gpio_set_dir(pin_flash_cs, GPIO_OUT);
    gpio_put(pin_flash_cs, 0);

    /* spi for CC1120 (RF, IC), Expansion*/
    spi_init(spi0, 100000);

    /* TODO@Zea as of December 23 2025: dma for display and radio module and stuff */
    /* i32 dma_channel = dma_claim_unused_channel(1);
    dma_channel_config dma_config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_config, 1);
    channel_config_set_write_increment(&dma_config, 0);
    channel_config_set_write_increment(&dma_config, 0); */

    /* main loop */
    for(u32 i = 0;;++i){
        poll_input();
        print_button_debounce_buffer();
        print_key_states();

        if((i& ((1<<5)-1)) == 0){
            gpio_put(pin_led_status, led_status_value);
            led_status_value = !led_status_value;
        }

        /* do display stuff(../hardware/Datasheets/AES200200A00-1.54ENRS(1).pdf)*/{
            if(!gpio_get(pin_display_busy)){
                gpio_put(pin_display_cs, 1);
            }

            spi_set_format(spi1, 9, SPI_CPOL_0, 0, SPI_MSB_FIRST);
            u16 data_bit = 1 << 8;

            u16 sw_reset = 0x12;
            spi_write16_blocking(spi1, &sw_reset, 1);
            sleep_ms(10);

            while(gpio_get(pin_display_busy)) sleep_ms(1);

            /* initalize gate settings*/
            {
                u16 set_gate_driver_output_control = 0x01;
                //select gate lines 9 bits, first 8 in 0 and then 1 in 1. 
                u16 gate_lines_0 =  (1<<7) | data_bit;
                u16 gate_lines_1 =  1 | data_bit;

                /*if 0 g0, g1, g2, g3... else g1, g0, g3, g2...*/
                u8 scan_staggered_bit = 0;
                /* if 1 : g0 g2 g4 ... g198, g1 g3 ... g199 */
                u8 scan_interlaced_bit = 0;
                /* if 1 g199 to g0*/
                u8 scan_backwards_bit = 0;
                u16 sequence_0 = (scan_staggered_bit << 2) | (scan_interlaced_bit << 1) | (scan_backwards_bit << 0) | data_bit; 


                /* set display rame size */
                /* define data entry sequence*/
                u16 set_data_entry_mode = 0x11;
                u8 define_data_entry_sequence = 0;
                u8 address_automatic_increment_decrement = 0;
                /* if 0 the address counter is updeted in the X direction if 1 its updated in the y direction*/
                u8 ram_direction = 0;

                u16 data_entry_mode_0 = (define_data_entry_sequence << 2) | (address_automatic_increment_decrement << 1) | (ram_direction << 0) | data_bit;


                u16 set_x_ram_address_start_end_position = 0x44;
                /* only has the first 6 bits of a byte*/
                u16 x_start_0 = 0;
                u16 x_end_0 = (1<<6)-1;


                u16 set_y_ram_address_start_end_position = 0x45;
                /* has 9 bits 8 in 0 1 in 1*/
                u16 y_start_0 = 0 | data_bit;
                u16 y_start_1 = 0 | data_bit;
                u16 y_end_0 = (1<<8)-1 | data_bit;
                u16 y_end_1 = 1 | data_bit;

                u16 buffer[] = {
                    set_gate_driver_output_control, gate_lines_0, gate_lines_1, sequence_0,
                    set_data_entry_mode, data_entry_mode_0,
                    set_x_ram_address_start_end_position, x_start_0, x_end_0,
                    set_y_ram_address_start_end_position, y_start_0, y_start_1, y_end_0, y_end_1
                };
                spi_write16_blocking(spi1, buffer, array_size(buffer));
            }

            /* load waveform LUT from OTP or by MCU 0x22 0x20 */
            u16 display_update_control_2_command = 0x22;
            /* 8 bits*/
            u8 operating_sequence_parameter_table[] = {
                0x80 /* Enable clock signal*/,
                0x01 /* Disable clock signal*/,
                0xc0 /* enable clock signale -> enable analog*/,
                0x03 /* disable analog -> disable clock signal*/,
                0x91 /* Enable clock signal -> laod LUT with Display mode 1 -> disable clock signal*/,
                0x99 /* enable clock signal -> load LUT with display mode 2 -> disable clock signal*/,
                0xb1 /* enable clock signal -> load temperature value -> load LUT with display mode 1*/,
                0xb9 /* enable clock signal -> load temperature value -> load LUT with display mode 2*/,
                0xc7 /* enable clock signal -> enable analog -> load LUT with display mode 1 -> disable analog -> disable OSC*/,
                0xcf /* enable clock signal -> enable analog -> load LUT with display mode 2 -> disable analog -> disable OSC*/,
                0xf7 /* enable clock signal -> enable analog -> load temperature value -> display with display mode 1 -> disable analog -> disable OSC*/,
                0xff /* enable clock signal -> enable analog -> load temperature value -> display with display mode 2 -> disable analog -> disable OSC*/,
            };

            /* BUSY will output high durring this command*/
            u16 master_activation_command = 0x20;

            /* load wavefrom LUT*/{
                u16 tempurature_sensor_selection = 0x18;
                /* 8 bits*/
                u16 sensor_control_0 = 0 | data_bit;


                u16 control_command_param_0 = operating_sequence_parameter_table[0] | data_bit;


                u16 buffer[] = {
                    tempurature_sensor_selection, sensor_control_0,
                    display_update_control_2_command, control_command_param_0,
                    master_activation_command
                };
                spi_write16_blocking(spi1, buffer, array_size(buffer));
            }
            while(gpio_get(pin_display_busy)) sleep_us(33);

            /* write image and drive display panel*/{
                u16 set_initial_ram_x_address = 0x4e;
                /* 6 bits*/
                u16 initial_ram_x_address = 0x00 | data_bit;

                u16 set_initial_ram_y_address = 0x4f;
                /* 9bits */
                u16 initial_ram_y_addres_0 = 0 | data_bit;
                u16 initial_ram_y_addres_1 = 0 | data_bit;

                /* Busy will be high*/
                u16 vcom_sense = 0x26;


                u16 buffer[] = {
                    set_initial_ram_x_address, initial_ram_x_address,
                    set_initial_ram_y_address, initial_ram_y_addres_0, initial_ram_y_addres_1,
                    vcom_sense, 
                };

                spi_write16_blocking(spi1, buffer, array_size(buffer));
                while(gpio_get(pin_display_busy)) sleep_us(33);

                u16 booster_soft_start_control   = 0x0c;
                union{
                    u8 u8;
                    struct{
                        u8 drive_strength:4;
                        u8 min_off_time_setting:4;
                    };
                } booster_phase_1, booster_phase_2, booster_phase_3;
                
                booster_phase_1.drive_strength = (1 << 4) - 1;
                booster_phase_1.min_off_time_setting = (1 << 4) - 1;

                booster_phase_2 = booster_phase_1;
                booster_phase_3 = booster_phase_1;

                /* 6 bits*/
                union{
                    u8 u8;
                    struct{
                        u8 phase_1:2;
                        u8 phase_2:2;
                        u8 phase_3:2;
                    };
                } phase_durrations;

                phase_durrations.u8 = 0;

                u16 phase_1 = booster_phase_1.u8 | data_bit;
                u16 phase_2 = booster_phase_2.u8 | data_bit;
                u16 phase_3 = booster_phase_3.u8 | data_bit;
                u16 durations = phase_durrations.u8 | data_bit;


                u16 display_command = operating_sequence_parameter_table[11];

                u16 buffer2[] = {
                    booster_soft_start_control, phase_1, phase_2, phase_3, durations,
                    display_update_control_2_command, display_command,
                    master_activation_command,
                };

                spi_write16_blocking(spi1, buffer, array_size(buffer));
                while(gpio_get(pin_display_busy)) sleep_us(33);
            }

            while(gpio_get(pin_display_busy)) sleep_us(10);
            /* set display rame size */
            {

            }


            c_str set_panel_boarder = "\x3c";
            spi_write_blocking(spi1, (u8 *)set_panel_boarder, strlen(set_panel_boarder));
            
            c_str sense_tempurature = "\x18";
            spi_write_blocking(spi1, (u8 *)sense_tempurature, strlen(sense_tempurature));

            c_str write_image_data = "\x4e";

        }



        /* TODO @Zea as of December 20 2025: make this try to keep the same interval between loops */
        sleep_ms(14);
        printf("loop:%"PRIu32"\r", i);
    }
}

