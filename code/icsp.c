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
    unsigned header = rw_multi(0, 0b001, 3) >> 2;

    // Read Data / possibly change to 8?
    // data = rw_multi(0, 0x80000000, 32);
    data = ((rw_multi(0, 0x80000000, 32) << 1) & ~0b1) | header;

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

    printk("Entering serial execution mode.\n");

    uint32_t statusVal = 0;

    send_command(MTAP_SW_MTAP);
    send_command(MTAP_COMMAND);

    statusVal = xfer_data(MCHP_STATUS);

    // Checking CPS is set
    printk("Checking CPS is set. Status: %b\n", statusVal);
    assert(bit_isset(statusVal, CPS));

    xfer_data(MCHP_ASSERT_RST);

    send_command(MTAP_SW_ETAP);
    send_command(ETAP_EJTAGBOOT);
    send_command(MTAP_SW_MTAP);
    send_command(MTAP_COMMAND);

    xfer_data(MCHP_DE_ASSERT_RST);    
    xfer_data(MCHP_EN_FLASH);

    printk("Entered serial execution mode.\n");
}

void xfer_instruction(uint32_t ins){
    send_command(ETAP_CONTROL);
    uint32_t controlVal;

    // check if processor access bit is set
    do{
        controlVal = xfer_data(0x0004C000);
    }while(!((controlVal >> PRACC) & 1)); // loop until access bit is set

    // select data register
    send_command(ETAP_DATA);

    // send the instruction
    xfer_data(ins);

    // tell cpu to execute instruction
    send_command(ETAP_CONTROL);
    xfer_data(0x0000C000);
}

uint32_t xfer_fast_data(uint32_t data) {
    // TMS Header
    rw_multi(0, 0b001, 3);

    // Send PrAcc
    unsigned opracc = rw_two_wire_four_phase(0, 0);

    // Read Data / possibly change to 8?
    data = ((rw_multi(0, 0x80000000, 32) << 1) & ~0b1) | opracc;

    // TMS Footer
    rw_multi(0, 0b01, 2);

    // printk("data:%b\r\n", data);
    return data;
}

void download_pe(void){
    // step 1: initialize BMXCON to 0x1f0040 
    xfer_instruction(0x3c04bf88);
    xfer_instruction(0x34842000);
    xfer_instruction(0x3c05001f);
    xfer_instruction(0x34a5004);
    xfer_instruction(0xac850000);

    // step 2: initialize BMXDKPBA to 0x800
    xfer_instruction(0x34050800);
    xfer_instruction(0xac850010);

    // step 3: initialize BMXDUDBA and BMXDUPBA to the value of BMXDRMSZ
    xfer_instruction(0x8C850040);
    xfer_instruction(0xac850020);
    xfer_instruction(0xac850030);

    // step 4: set up PIC32MX RAM address for PE
    xfer_instruction(0x3c04a000);
    xfer_instruction(0x34840800);

    // step 5: load the PE loader
    // pe loader opcode sequence from table 11-2 pg 21 of flash programming spec

    // 0x3c07dead
    xfer_instruction(0x3c063c07);
    xfer_instruction(0x34c6dead);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x3c06ff20
    xfer_instruction(0x3c063c06);
    xfer_instruction(0x34c6ff20);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x3c05ff20
    xfer_instruction(0x3c063c05);
    xfer_instruction(0x34c6ff20);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    printk("here1\r\n");

    // 0x8cc40000
    xfer_instruction(0x3c068cc4);
    xfer_instruction(0x34c60000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x8cc30000
    xfer_instruction(0x3c068cc3);
    xfer_instruction(0x34c60000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x1067000b
    xfer_instruction(0x3c061067);
    xfer_instruction(0x34c6000b);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x00000000
    xfer_instruction(0x3c060000);
    xfer_instruction(0x34c60000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x1060fffb
    xfer_instruction(0x3c061060);
    xfer_instruction(0x34c6fffb);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x00000000
    xfer_instruction(0x3c060000);
    xfer_instruction(0x34c60000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);
    
    printk("here2\r\n");

    // 0x8ca20000
    xfer_instruction(0x3c068ca2);
    xfer_instruction(0x34c60000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x2463ffff
    xfer_instruction(0x3c062463);
    xfer_instruction(0x34c6ffff);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0xac820000
    xfer_instruction(0x3c06ac82);
    xfer_instruction(0x34c60000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x24840004
    xfer_instruction(0x3c062484);
    xfer_instruction(0x34c60004);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x1460fffb
    xfer_instruction(0x3c061460);
    xfer_instruction(0x34c6fffb);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x00000000
    xfer_instruction(0x3c060000);
    xfer_instruction(0x34c60000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x1000fff3
    xfer_instruction(0x3c061000);
    xfer_instruction(0x34c6fff3);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x00000000
    xfer_instruction(0x3c060000);
    xfer_instruction(0x34c60000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    printk("here3\r\n");

    // 0x3c02a000
    xfer_instruction(0x3c063c02);
    xfer_instruction(0x34c6a000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x34420900
    xfer_instruction(0x3c063442);
    xfer_instruction(0x34c60900);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x00400008
    xfer_instruction(0x3c060040);
    xfer_instruction(0x34c60008);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // 0x00000000
    xfer_instruction(0x3c060000);
    xfer_instruction(0x34c60000);
    xfer_instruction(0xac860000);
    xfer_instruction(0x24840004);

    // step 6: jump to PE loader
    xfer_instruction(0x3c19a000);
    xfer_instruction(0x37390800);
    xfer_instruction(0x03200008);
    xfer_instruction(0x00000000);

    // step 7: load the PE using the PE loader
    // TODO: DEFINE THESE!!!!!!
    // uint32_t PE_ADDRESS = 0;
    // uint32_t PE_SIZE = 0;
    // uint32_t PE_OPCODE = 0;
    send_command(ETAP_FASTDATA);
    xfer_fast_data(PE_ADDRESS);
    xfer_fast_data(PE_SIZE);
    xfer_fast_data(PE_OPCODE);

    // step 8: jump to the PE
    xfer_fast_data(0x00000000);
    xfer_fast_data(0xDEAD0000);
}

void exit_programming_mode(void) {
    set_mode(0b011111);

    delay_us(15);
    gpio_set_off(MCLR);
}