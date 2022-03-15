#include "jtag.h"
#include "libc/bit-support.h"

void configure_io(void) {
    gpio_set_output(MCLR);
    gpio_set_output(TCK);
    gpio_set_output(TDI);
    gpio_set_output(TMS);
    gpio_set_input(TDO);
    gpio_pud_off(TDO);
    gpio_set_off(MCLR);
    gpio_set_off(TCK);
    gpio_set_off(TDI);
    gpio_set_off(TMS);
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
    delay_us(300);
    set_mode(0b011111);
    delay_us(300);
    printk("Sending MTAP_SW_MTAP command.\n");
    send_command(MTAP_SW_MTAP); // 0b00100
    delay_us(300);
    set_mode(0b011111);
    delay_us(300);
    printk("Sending MTAP_COMMAND command.\n");
    send_command(MTAP_COMMAND); // 0b00111
    delay_us(300);

    printk("About to read device status.\n");

    do {
        statusVal = xfer_data(MCHP_STATUS);
        delay_us(1000);
    } while(!bit_isset(statusVal, CFGRDY) || bit_isset(statusVal, FCBUSY)); // bit 2 (FCBUSY) needs to be 0 and bit 3 (CFGRDY) needs to be 1

    printk("Checked device status! Status: %b\n", statusVal);
}

void set_mode(uint8_t mode){
    gpio_set_off(TDI); // TDI set to 0
    gpio_set_off(TCK); // set clock off initially
    send_nbits(TMS, mode, 6);
}

void send_command(uint8_t cmd){
    gpio_set_off(TCK);
    // send TMS header = 1, 1, 0, 0 to select shift IR state
    send_nbits(TMS, 0b0011, 4);

    // clock in the 5-bit command LSB-first
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

    // bits 30:0
    for (unsigned i = 0; i < XFER_WIDTH - 1; i++) {
        gpio_write(TDI, (data >> i) & 1);
        in_data |= (gpio_read(TDO) << i);
        clock_in();
    }
    // bit 31 + TMS = 1
    gpio_set_on(TMS);
    gpio_write(TDI, (data >> (XFER_WIDTH - 1)) & 1);
    in_data |= (gpio_read(TDO) << (XFER_WIDTH - 1));
    clock_in();

    // TMS Footer 1, 0
    send_nbits(TMS, 0b01, 2);

    printk("in_data:%b\r\n", in_data);
    return in_data;
}


static void send_nbits(unsigned pin, unsigned val, unsigned n) {
    for(unsigned i = 0; i < n; i++){
        gpio_write(pin, (val >> i) & 1);
        clock_in();
    }
}

static void clock_in(void){
    delay_us(TCK_DELAY);
    gpio_set_on(TCK);
    delay_us(TCK_DELAY);
    gpio_set_off(TCK);
    delay_us(TCK_DELAY);
}

void erase_device(void){
    send_command(MTAP_SW_MTAP);
    send_command(MTAP_COMMAND);
    xfer_data(MCHP_ERASE);
    uint32_t statusVal;
    do{
        statusVal = xfer_data(MCHP_STATUS);
        delay_ms(1);
    }while(!bit_isset(statusVal, CFGRDY) || bit_isset(statusVal, FCBUSY)); // FCBUSY (bit 2) must be 0 and CFGRDY (bit 3) must be 1

    printk("Erased device! Current Device Status: %b\n", statusVal);
}

void enter_serial_execution_mode(void){
    send_command(MTAP_SW_MTAP);
    send_command(MTAP_COMMAND);
    uint32_t statusVal = xfer_data(MCHP_STATUS);
    if(!bit_isset(statusVal, CPS)){
        printk("Device must be erased first.\r\n");
        return;
    }
    send_command(MTAP_SW_ETAP);
    send_command(ETAG_EJTAGBOOT);
    gpio_set_on(MCLR);
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

uint32_t xfer_fast_data(uint32_t data){
    // send TMS header 100
    gpio_set_off(TCK);
    send_nbits(TMS, 0b001, 3);

    // clock in input value of processor access (PrAcc) bit which is 0
    gpio_set_off(TDI);

    uint32_t in_data = 0;

    // bits 30:0
    for (unsigned i = 0; i < XFER_WIDTH - 1; i++) {
        gpio_write(TDI, (data >> i) & 1);
        in_data |= (gpio_read(TDO) << i);
        clock_in();
    }

    // bit 31 + TMS = 1
    gpio_set_on(TMS);
    gpio_write(TDI, (data >> (XFER_WIDTH - 1)) & 1);
    in_data |= (gpio_read(TDO) << (XFER_WIDTH - 1));
    clock_in();

    // TMS Footer 1, 0
    send_nbits(TMS, 0b01, 2);

    printk("in_data:%b\r\n", in_data);
    return in_data;
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
    uint32_t PE_ADDRESS = 0;
    uint32_t PE_SIZE = 0;
    uint32_t PE_OPCODE = 0;
    send_command(ETAP_FASTDATA);
    xfer_fast_data(PE_ADDRESS);
    xfer_fast_data(PE_SIZE);
    xfer_fast_data(PE_OPCODE);

    // step 8: jump to the PE
    xfer_fast_data(0x00000000);
    xfer_fast_data(0xDEAD0000);
}