#include "debug_pulse.h"

#include "pico/stdlib.h"
#define DEBUG_PULSE_MAX_COUNT (100)

struct debug_pin_s {
    uint pin;
};

static struct debug_pin_s debug_pins[] = {{4}, {5}, {6}, {7}, {8}, {9}};


#define NUM_PINS (sizeof(debug_pins)/sizeof(debug_pins[0]))
static int debug_pulse_is_initialized = 0;
static int debug_pulse_num_pins = NUM_PINS;
static int pin_state[NUM_PINS];
static uint32_t global_pin_mask = 0;

void debug_pulse_init(void)
{
    if (debug_pulse_is_initialized)
        return;
    global_pin_mask = (1<<NUM_PINS)-1;
    for (int i = 0; i < debug_pulse_num_pins; i++) {
        pin_state[i] = 0;
        gpio_init(debug_pins[i].pin);
        gpio_set_dir(debug_pins[i].pin, GPIO_OUT);
    }
    debug_pulse_is_initialized = 1;
}
void debug_pulse_delay(int count) {
    volatile int x = count;
    while (x--);
}

static struct debug_pin_s map(int pin) {
    if (!debug_pulse_is_initialized)
        debug_pulse_init();
    if (pin >=0 && pin < debug_pulse_num_pins) {
        return debug_pins[pin];
    } else {
        return debug_pins[0];
    }
}

void debug_toggle(int pin) {
    struct debug_pin_s p = map(pin);
    pin_state[pin] = !pin_state[pin];
    gpio_put(p.pin, pin_state[pin]);
}

void debug_pin_high(int pin) {
    struct debug_pin_s p = map(pin);
    pin_state[pin] = 1;
    gpio_put(p.pin, pin_state[pin]);
}
void debug_pin_low(int pin) {
    struct debug_pin_s p = map(pin);
    pin_state[pin] = 0;
    gpio_put(p.pin, pin_state[pin]);
}

void debug_pulse(int pin, int count)
{
    struct debug_pin_s p = map(pin);
    if (count > DEBUG_PULSE_MAX_COUNT)
        count = DEBUG_PULSE_MAX_COUNT;
    gpio_put(p.pin, 0);
    for (int i = 0; i < count ; i++) {
        gpio_put(p.pin, 1);
        debug_pulse_delay(1);
        gpio_put(p.pin, 0);
        debug_pulse_delay(1);
    }
    debug_pulse_delay(5);
}

// Set all the pins defined in 'pins', as masked by pin_mask
// this lets you set any set of debug pins without affecting
// others.
void debug_set_pins(uint32_t pins, uint32_t pin_mask)
{
    pin_mask |= global_pin_mask;
    for (uint32_t i = 0; i < NUM_PINS; i++, pins >>= 1 ) {
        if (pins & 0x1)
            debug_pin_high(i);
        else
            debug_pin_low(i);
    }
}

