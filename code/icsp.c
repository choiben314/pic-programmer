#include "icsp.h"
#include "libc/bit-support.h"

void configure_io(void) {
    gpio_set_output(MCLR);
    gpio_set_output(PGD);
    gpio_set_output(PGC);
    gpio_set_off(MCLR);
    gpio_set_off(PGD);
    gpio_set_off(PGC);
}

// Enter enhanced ICSP
void enter_programming_mode(void) {
    gpio_set_on(MCLR);
    delay_us(10);
    gpio_set_off(MCLR);
    delay_us(300);

    // send KEY_SEQ
    send_rising_edge_sequence(PGD, KEY_SEQ, 32);
    
    gpio_set_on(MCLR);
    delay_us(1);
}

// Read device status
void read_device_status(void) {
    uint32_t statusVal = 0;
    
    printk("Entering programming mode.\n");
    enter_programming_mode();

    printk("Setting mode.\n");
    set_mode(0b011111);
    printk("Sending MTAP_SW_MTAP command.\n");
    send_command(MTAP_SW_MTAP); // 0b00100
    set_mode(0b011111);
    printk("Sending MTAP_COMMAND command.\n");
    send_command(MTAP_COMMAND); // 0b00111

    printk("About to read device status.\n");

    do {
        statusVal = xfer_data(MCHP_STATUS);
        delay_ms(10);
    } while(!bit_isset(statusVal, CFGRDY) || bit_isset(statusVal, FCBUSY)); // bit 2 (FCBUSY) needs to be 0 and bit 3 (CFGRDY) needs to be 1

    printk("Checked device status! Status: %b\n", statusVal);
}

void send_rising_edge_sequence(unsigned pin, unsigned seq, unsigned n) {
    gpio_set_output(PGD);
    for (unsigned i = 0; i < n; i++) {
        delay_us(1);
        gpio_write(pin, (seq >> (n - i - 1)) & 1);
        delay_us(1);
        gpio_set_on(PGC);
        delay_us(1);
        gpio_set_off(PGC);
    }
    delay_us(1);
}

// Returns TDO
unsigned rw_two_wire_four_phase(unsigned tdi, unsigned tms) {

    gpio_set_output(PGD);
    delay_us(1);

    // clock high and write tdi
    gpio_set_on(PGC);
    gpio_write(PGD, tdi);

    // falling edge
    delay_us(1);
    gpio_set_off(PGC);
    delay_us(1);

    // clock high and write tms
    gpio_set_on(PGC);
    gpio_write(PGD, tms);

    // falling edge
    delay_us(1);
    gpio_set_off(PGC);
    delay_us(1);

    // clock high and set PGD as input
    gpio_set_input(PGD);
    gpio_set_on(PGC);

    // falling edge
    delay_us(1);
    gpio_set_off(PGC);
    delay_us(1);

    // Read tdo while clock low
    unsigned tdo = gpio_read(PGD);
    delay_us(1);

    // clock high then low
    gpio_set_on(PGC);
    delay_us(1);
    gpio_set_off(PGC);
    
    delay_us(1);

    return tdo;
}

// args: LSB First
unsigned rw_multi(unsigned tdi_v, unsigned tms_v, unsigned n) {
    unsigned tdo_v = 0;
    for (unsigned i = 0; i < n; i++) {
        tdo_v |= (rw_two_wire_four_phase((tdi_v >> i) & 1, (tms_v >> i) & 1) << i);
    }

    return tdo_v;    
}

void set_mode(uint8_t mode){
    rw_multi(0, mode, 6);
}

void send_command(uint8_t cmd){
    // TMS Header
    rw_multi(0, 0b0011, 4);

    // Command
    rw_multi(cmd, 0b10000, 5);

    // TMS Footer
    rw_multi(0, 0b01, 2);
}

uint32_t xfer_data(uint32_t data) {
    // TMS Header
    rw_multi(0, 0b001, 3);

    // Read Data / possibly change to 8?
    data = rw_multi(0, 0x80000000, 32);

    // TMS Footer
    rw_multi(0, 0b01, 2);

    // printk("data:%b\r\n", data);
    return data;
}

void erase_device(void) {
    uint32_t statusVal = 0;

    send_command(MTAP_SW_MTAP);
    send_command(MTAP_COMMAND);

    printk("About to chip erase.\n");
    xfer_data(MCHP_ERASE);
    
    delay_ms(90);

    do {
        statusVal = xfer_data(MCHP_STATUS);
        delay_us(1);
    } while(!bit_isset(statusVal, CFGRDY) || bit_isset(statusVal, FCBUSY)); // bit 2 (FCBUSY) needs to be 0 and bit 3 (CFGRDY) needs to be 1

    printk("Chip erase complete! Status: %b\n", statusVal);
}

void enter_serial_execution_mode(void) {
    uint32_t statusVal = 0;

    send_command(MTAP_SW_MTAP);
    send_command(MTAP_COMMAND);

    statusVal = xfer_data(MCHP_STATUS);

    // Checking CPS is set
    printk("Checking CPS is set. Status: %b\n", statusVal);
    assert(bit_isset(statusVal, CPS));

    // printk("About to chip erase.\n");
    // xfer_data(MCHP_ERASE);
    
    // delay_ms(90);

    // do {
    //     statusVal = xfer_data(MCHP_STATUS);
    //     delay_ms(10);
    // } while(!bit_isset(statusVal, CFGRDY) || bit_isset(statusVal, FCBUSY)); // bit 2 (FCBUSY) needs to be 0 and bit 3 (CFGRDY) needs to be 1

    // printk("Chip erase complete! Status: %b\n", statusVal);
}