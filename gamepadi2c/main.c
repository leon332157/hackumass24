#include <pico/i2c_slave.h>
#include <stdio.h>

#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "boards/seeed_xiao_rp2040.h"

#define I2C_RATE 400 * 1000 // 400KHz
#define I2C_ADDR 0x12
#define DEADZONE 20

struct JoyStickState {
    uint8_t x;
    uint8_t y;
};

struct JoyStickState joystick_state;
uint8_t button_state;

enum ButtonMap {
    BUTTON_X = 1 << 0,
    BUTTON_A = 1 << 1,
    BUTTON_B = 1 << 2,
    BUTTON_Y = 1 << 3,
    BUTTON_HAT = 1 << 4,
    BUTTON_L1 = 1 << 5,
    BUTTON_L2 = 1 << 6,
    RESERVED = 1 << 7
};

struct packet {
    uint8_t button_state;
    uint8_t joystickX_val;
    uint8_t joystickY_val;
};

static inline void init_led_pin() {
    gpio_init(17);
    gpio_set_dir(17, GPIO_OUT);
    gpio_put(17, 1);
    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);
    gpio_put(16, 1);
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);
}

static inline void red_led_flash(uint32_t ms, uint8_t times) {
    for (; times > 0; times--) {
        gpio_put(17, 0);
        sleep_ms(ms);
        gpio_put(17, 1);
        sleep_ms(ms);
    }
}

static inline void green_led_flash(uint32_t ms, uint8_t times) {
    for (; times > 0; times--) {
        gpio_put(16, 0);
        sleep_ms(ms);
        gpio_put(16, 1);
        sleep_ms(ms);
    }
}

static inline void blue_led_flash(uint32_t ms, uint8_t times) {
    for (; times > 0; times--) {
        gpio_put(25, 0);
        sleep_ms(ms);
        gpio_put(25, 1);
        sleep_ms(ms);
    }
}

static void send_packet() {
    struct packet packet;
    packet.button_state = button_state;
    packet.joystickX_val = joystick_state.x;
    packet.joystickY_val = joystick_state.y;
    i2c_write_raw_blocking(i2c1, (uint8_t*)&packet, sizeof(packet));
}

static void i2c_slave_handler(i2c_inst_t* i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: // master has written some data
        break;
    case I2C_SLAVE_REQUEST: // master is requesting data
        send_packet();
        // green_led_flash(100,2);
        break;
    case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
        // blue_led_flash(100,2);
        break;
    default:
        break;
    }
}

static void setup_slave() {
    gpio_init(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);

    gpio_init(PICO_DEFAULT_I2C_SCL_PIN);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    i2c_init(i2c1, I2C_RATE);
    // configure I2C0 for slave mode
    i2c_slave_init(i2c1, I2C_ADDR, &i2c_slave_handler);
}

inline static void setup_adc() {
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    adc_gpio_init(27);
}

inline static void setup_btns() {
    gpio_init(3); // X
    gpio_init(4); // A
    gpio_init(2); // B
    gpio_init(1); // Y
    gpio_set_dir(0, GPIO_IN); // HAT
    gpio_pull_up(0);
    gpio_init(28); // L1
    gpio_init(29); // L2
}

inline static void read_btns() {
    uint32_t data = gpio_get_all();
    if (data & 0x1 << 3) {
        button_state |= BUTTON_X;
    } else {
        button_state &= ~BUTTON_X;
    }
    if (data & 0x1 << 4) {
        button_state |= BUTTON_A;
    } else {
        button_state &= ~BUTTON_A;
    }
    if (data & 0x1 << 2) {
        button_state |= BUTTON_B;
    } else {
        button_state &= ~BUTTON_B;
    }
    if (data & 0x1 << 1) {
        button_state |= BUTTON_Y;
    } else {
        button_state &= ~BUTTON_Y;
    }
    if (0 == data & 0x0) {
        button_state |= BUTTON_HAT;
    } else {
        button_state &= ~BUTTON_HAT;
    }
    if (data & 0x1 << 28) {
        button_state |= BUTTON_L1;
    } else {
        button_state &= ~BUTTON_L1;
    }
    if (data & 0x1 << 29) {
        button_state |= BUTTON_L2;
    } else {
        button_state &= ~BUTTON_L2;
    }
    printf("Button state: %b\n", button_state);
    // sleep_ms(10);
    sleep_us(100);
}

inline static void read_axis() {
    adc_select_input(0);
    uint16_t adc_x_raw = adc_read();
    adc_select_input(1);
    uint16_t adc_y_raw = adc_read();
    printf("X: %u, Y: %u X_scaled: %u, Y_scaled: %u ", adc_x_raw, adc_y_raw, adc_x_raw / 16, adc_y_raw / 16);
    joystick_state.x = adc_x_raw >> 4;
    joystick_state.y = adc_y_raw >> 4;
    // sleep_ms(50);
    // sleep_us(100);
}

void core1_task() {
    while (true) {
        read_axis();
        read_btns();
    }
}

int main() {
    stdio_init_all();
    init_led_pin();
    setup_slave();
    setup_adc();
    setup_btns();
    multicore_launch_core1(core1_task);
    while (true) {
        /*
        if (button_state & BUTTON_X) {
            red_led_flash(100, 1);
        } else {
            gpio_put(17, 1);
        }
        if (button_state & BUTTON_A) {
            green_led_flash(100, 1);
        } else {
            gpio_put(16, 1);
        }
        if (button_state & BUTTON_B) {
            gpio_put(25, 0);
        } else {
            gpio_put(25, 1);
        }
        if (button_state & BUTTON_Y) {
            gpio_put(17, 0);
            gpio_put(16, 0);
            gpio_put(25, 0);
        } else {
            gpio_put(17, 1);
            gpio_put(16, 1);
            gpio_put(25, 1);
        }
        if (button_state & BUTTON_HAT) {
            gpio_put(17, 0);
            gpio_put(16, 0);
        } else {
            gpio_put(17, 1);
            gpio_put(16, 1);
        }*/
        sleep_us(100);
    }
}
