/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32 
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"

// see broadcomm documents for magic addresses.
#define GPIO_BASE 0x20200000
static const unsigned gpio_set0  = (GPIO_BASE + 0x1C);
static const unsigned gpio_clr0  = (GPIO_BASE + 0x28);
static const unsigned gpio_lev0  = (GPIO_BASE + 0x34);
static const unsigned gpio_fsel0 = (GPIO_BASE + 0x00);

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//



int valid_pin(unsigned pin) {
    return !(pin >= 32 && pin != 47);
}

// set GPIO function for <pin> (input, output, alt...).  settings for other
// pins should be unchanged.
void gpio_set_function(unsigned pin, gpio_func_t function) {
    if (!valid_pin(pin))
        return;

    if (function < GPIO_FUNC_INPUT || function > GPIO_FUNC_ALT3)
        return;

    uintptr_t temp = (uintptr_t) gpio_fsel0;
    unsigned int * gpio_fsel = (unsigned int *) temp;
    unsigned int gpio_fsel_reg = (uintptr_t) &(gpio_fsel[pin / 10]);
    unsigned int offset = 3 * (pin % 10);
    unsigned int write = (GET32(gpio_fsel_reg) & ~(0b111 << offset)) | (function << offset);
    PUT32(gpio_fsel_reg, write);
    
}

// set <pin> to be an output pin.  
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you 
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
    // implement this
    // use <gpio_fsel0>

    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
    // implement this
    // use <gpio_set0>

    if (!valid_pin(pin))
        return;

    PUT32(gpio_set0, 1 << pin);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
    // implement this
    // use <gpio_clr0>

    if (!valid_pin(pin))
        return;
        
    PUT32(gpio_clr0, 1 << pin);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(v)
       gpio_set_on(pin);
    else
       gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> to input.
void gpio_set_input(unsigned pin) {
    // implement.

    gpio_set_function(pin, GPIO_FUNC_INPUT);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
    unsigned v = 0;

    // implement.

    if (!valid_pin(pin))
        return -1;

    v = (GET32(gpio_lev0) >> pin) & 0b1;

    return DEV_VAL32(v);
}
