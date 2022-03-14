// engler, cs140 put your gpio implementations in here.
#include "rpi.h"

#define GPIO_BASE 0x20200000
#define GPIO_INT_BASE 0x2000B000

static const unsigned gpio_eds0  = (GPIO_BASE + 0x40);
static const unsigned gpio_eds1  = (GPIO_BASE + 0x44);
static const unsigned gpio_ren0  = (GPIO_BASE + 0x4C);
static const unsigned gpio_ren1  = (GPIO_BASE + 0x50);
static const unsigned gpio_fen0  = (GPIO_BASE + 0x58);
static const unsigned gpio_fen1 = (GPIO_BASE + 0x5C);


static const unsigned gpio_irq_2 = (GPIO_INT_BASE + 0x208);
static const unsigned gpio_irq_2_en = (GPIO_INT_BASE + 0x214);

// gpio_int_rising_edge and gpio_int_falling_edge (and any other) should
// call this routine (you must implement) to setup the right GPIO event.
// as with setting up functions, you should bitwise-or in the value for the 
// pin you are setting with the existing pin values.  (otherwise you will
// lose their configuration).  you also need to enable the right IRQ.   make
// sure to use device barriers!!
int is_gpio_int(unsigned gpio_int) {
    dev_barrier();

    int pending;

    if (gpio_int >= GPIO_INT0 && gpio_int <= GPIO_INT3) {
        pending = GET32(gpio_irq_2) & (1 << (gpio_int - 32));
    } else {
        panic("gpio_int param not valid GPIO_INT0...3 input");
    }

    dev_barrier();

    return pending;
}


// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
    dev_barrier();

    PUT32(gpio_irq_2_en, 0b1111 << (GPIO_INT0 - 32));

    if (pin < 32) {
        PUT32(gpio_ren0, GET32(gpio_ren0) | (1 << pin));
    } else if (pin == 47) {
        PUT32(gpio_ren1, GET32(gpio_ren1) | (1 << (pin - 32)));
    }
    
    dev_barrier();
}

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
    dev_barrier();

    PUT32(gpio_irq_2_en, 0b1111 << (GPIO_INT0 - 32));

    if (pin < 32) {
        PUT32(gpio_fen0, GET32(gpio_fen0) | (1 << pin));
    } else if (pin == 47) {
        PUT32(gpio_fen1, GET32(gpio_fen1) | (1 << (pin - 32)));
    }

    dev_barrier();
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    dev_barrier();

    int status = 0;
    
    if (pin < 32) {
        status = GET32(gpio_eds0) & (1 << pin);
    } else if (pin == 47) {
        status = GET32(gpio_eds1) & (1 << (pin - 32));
    }

    dev_barrier();

    return status;
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    dev_barrier();

    if (pin < 32) {
        PUT32(gpio_eds0, (1 << pin));
    } else if (pin == 47) {
        PUT32(gpio_eds1, (1 << (pin - 32)));
    }

    dev_barrier();
}
