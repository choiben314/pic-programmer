#include "jtag.h"
#include "libc/bit-support.h"

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
static void send_nbits(unsigned pin, unsigned val, unsigned n);


void read_device_status(void) {
    uint32_t statusVal;
    
    printk("Setting mode.\n");
    set_mode(0b011111);
    printk("Sending MTAP_SW_MTAP command.\n");
    send_command(MTAP_SW_MTAP);
    printk("Sending MTAP_COMMAND command.\n");
    send_command(MTAP_COMMAND);

    printk("About to read device status.\n");

    do {
        statusVal = xfer_data(MCHP_STATUS);
    } while(!bit_isset(statusVal, CFGRDY) || bit_isset(statusVal, FCBUSY));

    printk("Checked device status! Status: %b\n", statusVal);
}

void set_mode(uint8_t mode){
    gpio_set_off(TDI); // TDI set to 0
    gpio_set_off(TCK); // set clock off initially
    send_nbits(TMS, mode, 6);
}

void send_command(uint8_t cmd){
    // send TMS header = 1, 1, 0, 0 to select shift IR state
    send_nbits(TMS, 0b0011, 4);

    // clock in the 5-bit command
    gpio_set_off(TMS); // hold TMS low
    for(uint8_t i=0; i < 5; i++){
        if(i == 4){
            gpio_set_on(TMS); // set TMS high for MSB
        }
        gpio_write(TDI, (cmd >> i) & 1);
        clock_in();
    }

    // send TMS footer = 1, 0
    send_nbits(TMS, 0b01, 2);
}

uint32_t xfer_data(uint32_t data) {
    // TMS Header 1, 0, 0
    gpio_set_off(TCK);
    send_nbits(TMS, 0b001, 3);

    uint32_t in_data = 0;

    // bits 31:0
    for (unsigned i = 0; i < XFER_WIDTH - 1; i++) {
        gpio_write(TDI, (data >> i) & 1);
        in_data |= (gpio_read(TDO) << i);
        clock_in();
    }
    // bit 32 + TMS = 1
    gpio_set_on(TMS);
    gpio_write(TDI, (data >> (XFER_WIDTH - 1)) & 1);
    in_data |= (gpio_read(TDO) << (XFER_WIDTH - 1));
    clock_in();

    // TMS Footer 1, 0
    send_nbits(TMS, 0b01, 2);

    return in_data;
}


static void send_nbits(unsigned pin, unsigned val, unsigned n) {
    for(unsigned i = 0; i < n; i++){
        gpio_write(pin, (val >> i) & 1);
        clock_in();
    }
}

static void clock_in(void){
    gpio_set_on(TCK);
    delay_us(TCK_DELAY);
    gpio_set_off(TCK);
    delay_us(TCK_DELAY);
}