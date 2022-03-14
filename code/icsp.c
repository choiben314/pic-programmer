#include "icsp.h"


void configure_io(void) {
    gpio_set_output(MCLR);
    gpio_set_output(TCK);
    gpio_set_output(TDI);
    gpio_set_input(TDO);
    gpio_set_off(MCLR);
    gpio_set_off(TCK);
    gpio_set_off(TDI);
}

// Enter enhanced ICSP

void enter_programming_mode(void) {
    unimplemented();
    // gpio_set_on(PIN_MCLR);
    // delay_us(10);
    // gpio_set_off(PIN_MCLR);
    // delay_us(300);

    // // send KEY_SEQ
    
    // gpio_set_on(PIN_MCLR);
    // delay_us(1);
}

// MCLR pin driven high, then low
// 32-bit key sequence is clocked into PGDx
// MCLR driven high with specific period of time and held


// Clock goes high
// Set PGD
// Delay by some amount
// Clock goes low
// 


// Read device status

// SetMode (6'b011111) to force TAP controller into Run Test/Idle state

// SendCommand(MTAP_SW_MTAP)
// SendCommand(MTAP_COMMAND)
// statusVal = XferData(MCHP_STATUS)
// If CFGRDY (statusVal<3>) is 0 and FCBUSY (statusVal<2>) is 1
    // delay 10ms
    // read Xfer data again
// Done.

void set_mode(uint8_t mode){
    gpio_set_off(TDI); // TDI set to 0
    for(uint8_t i=0; i < 6; i++){
        gpio_set_on(TCK);
        // set TMS to i-th bit of mode
        if(mode >> i & 0b1){
            gpio_set_on(TMS);
        }else{
            gpio_set_off(TMS);
        }
        delay_us(TCK_DELAY);
        gpio_set_off(TCK);
        delay_us(TCK_DELAY);
    }
}