#ifndef _DEBUG_PULSE_H_
#define _DEBUG_PULSE_H_

#include <stdint.h>

void debug_pulse_init(void);
void debug_pulse(int pin, int count);
void debug_toggle(int pin);
void debug_pin_high(int pin);
void debug_pin_low(int pin);
void debug_set_pins(uint32_t pins, uint32_t pin_mask);


#endif
