// implement:
//  void uart_init(void)
//
//  int uart_can_getc(void);
//  int uart_getc(void);
//
//  int uart_can_putc(void);
//  void uart_putc(unsigned c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"

#define AUXENB 0x20215004
#define AUX_MU_IO_REG 0x20215040
#define AUX_MU_IER_REG 0x20215044
#define AUX_MU_IIR_REG 0x20215048
#define AUX_MU_LCR_REG 0x2021504C
#define AUX_MU_LSR_REG 0x20215054
#define AUX_MU_CNTL_REG 0x20215060
#define AUX_MU_STAT_REG 0x20215064
#define AUX_MU_BAUD 0x20215068

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {

    dev_barrier();

    // initialize gpio functions
    gpio_set_function(14, GPIO_FUNC_ALT5);
    gpio_set_function(15, GPIO_FUNC_ALT5);

    dev_barrier();

    // turn on uart in aux
    PUT32(AUXENB, GET32(AUXENB) | 0x1);

    dev_barrier();

    // disable tx/rx
    PUT32(AUX_MU_CNTL_REG, 0);

    // figure out which registers you can ignore

    // find and clear all parts of its state(e.g., FIFO queues), disable interrupts
    PUT32(AUX_MU_IIR_REG, 0b110);
    PUT32(AUX_MU_IER_REG, 0);

    // configure 115200 baud, 8 bits, 1 start bit, 1 stop bit. no flow control.
    PUT32(AUX_MU_BAUD, 270);
    PUT32(AUX_MU_LCR_REG, 0b11);
    // PUT32(AUX_MU_CNTL_REG, GET32(AUX_MU_CNTL_REG) & ~0b1100);

    // enable tx/rx
    PUT32(AUX_MU_CNTL_REG, 0b11);

    dev_barrier();
}

// disable the uart.
void uart_disable(void) {
    dev_barrier();

    uart_flush_tx();
    
    dev_barrier();

    // turn off uart in aux
    PUT32(AUXENB, GET32(AUXENB) & ~0x1);
    dev_barrier();
}

// 1 = at least one byte on rx queue, 0 otherwise
static int uart_can_getc(void) {
    dev_barrier();
    int ret = GET32(AUX_MU_STAT_REG) & 1;
    dev_barrier();
    
    return ret;
    
}

// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_getc(void) {
    dev_barrier();
    
    while (!uart_can_getc());

    int ret = GET32(AUX_MU_IO_REG) & 0xFF;
    
    dev_barrier();
    
    return ret;
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_putc(void) {
    dev_barrier();
    int ret = GET32(AUX_MU_STAT_REG) & 0b10;
    return ret;

    dev_barrier();
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
void uart_putc(unsigned c) {
    dev_barrier();

    while (!uart_can_putc());
   
    PUT32(AUX_MU_IO_REG, c & 0xFF);

    dev_barrier();
}

// simple wrapper routines useful later.

// a maybe-more intuitive name for clients.
int uart_has_data(void) {
    return uart_can_getc();
}


// return -1 if no data, otherwise the byte.
int uart_getc_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_getc();
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
    dev_barrier();
    int c = (GET32(AUX_MU_STAT_REG) >> 9) & 1;
    dev_barrier();
    return c;
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty())
        ;
}
