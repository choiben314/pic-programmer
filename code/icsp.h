#include "rpi.h"

#define HI_TIME 50
#define LO_TIME 50
#define XFER_WIDTH 32

#define TCK_DELAY 10 // delay time in us 
#define MCLR 26 // pin 1 on PIC
#define PGD 5 // pin 4 on PIC
#define PGC 6 // pin 5 on PIC

#define MTAP_SW_MTAP 0x04
#define MTAP_SW_ETAP 0x05
#define MTAP_COMMAND 0x07

#define ETAP_EJTAGBOOT 0x0C
#define ETAP_CONTROL 0x0A
#define ETAP_DATA 0x09
#define ETAP_FASTDATA 0x0E

#define MCHP_STATUS 0x00
#define MCHP_ERASE 0xFC
#define MCHP_ASSERT_RST 0xD1
#define MCHP_DE_ASSERT_RST 0xD0
#define MCHP_EN_FLASH 0xFE

#define CFGRDY 3
#define FCBUSY 2
#define CPS 7
#define PRACC 18

#define PE_ADDRESS 0x0
#define PE_SIZE 0
#define PE_OPCODE 0

#define KEY_SEQ 0b01001101010000110100100001010000

// Helpers
void send_rising_edge_sequence(unsigned pin, unsigned seq, unsigned n);
unsigned rw_two_wire_four_phase(unsigned tdi, unsigned tms);
unsigned rw_multi(unsigned tdi_v, unsigned tms_v, unsigned n);

// Programming Steps
void configure_io(void);

// Only needed for two-wire mode
void enter_programming_mode(void);

void read_device_status(void);

void set_mode(uint8_t mode);

void send_command(uint8_t cmd);

uint32_t xfer_data(uint32_t data);

void erase_device(void);

void enter_serial_execution_mode(void);

void xfer_instruction(uint32_t ins);

uint32_t xfer_fast_data(uint32_t data);

void download_pe(void);

void exit_programming_mode(void);