#include "jtag.h"



// Enter enhanced ICSP

// MCLR pin driven high, then low
// 32-bit key sequence is clocked into PGDx
// MCLR driven high with specific period of time and held




// Read device status

// SetMode (6'b011111) to force TAP controller into Run Test/Idle state

// SendCommand(MTAP_SW_MTAP)
// SendCommand(MTAP_COMMAND)
// statusVal = XferData(MCHP_STATUS)
// If CFGRDY (statusVal<3>) is 0 and FCBUSY (statusVal<2>) is 1
    // delay 10ms
    // read Xfer data again
// Done.
static void clock_in(void);

void set_mode(uint8_t mode){
    gpio_set_off(TDI); // TDI set to 0
    gpio_set_off(TCK); // set clock off initially
    for(uint8_t i=0; i < 6; i++){
        // set TMS to i-th bit of mode
        gpio_write(TMS, mode >> i & 1);
        clock_in();
    }
}

void send_command(uint8_t cmd){
    gpio_set_off(TCK);
    // send TMS header = 1100 to select shift IR state
    for(uint8_t i=0; i < 4; i++){
        // set first two bits to 1 and last two to 0
        gpio_write(TMS, i < 2 ? 1: 0);
        clock_in();
    }

    // clock in the 5-bit command
    gpio_set_off(TMS); // hold TMS low
    for(uint8_t i=0; i < 5; i++){
        if(i == 4){
            gpio_set_on(TMS); // set TMS high for MSB
        }
        gpio_write(TDI, cmd >> i & 1);
        clock_in();
    }

    // send TMS footer = 10
    gpio_set_on(TMS);
    clock_in();
    gpio_set_off(TMS);
    clock_in();

}

static void clock_in(void){
    gpio_set_on(TCK);
    delay_us(TCK_DELAY);
    gpio_set_off(TCK);
    delay_us(TCK_DELAY);
}