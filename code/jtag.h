#include "rpi.h"

#define HI_TIME 50
#define LO_TIME 50
#define XFER_WIDTH 32

#define TCK_DELAY 10 // delay time in us 
#define MCLR 26 // pin 1 on PIC
#define TDI 5 // pin 16 on PIC
#define TCK 6 // pin 17 on PIC
#define TDO 13 // pin 18 on PIC
#define TMS 19 // pin 22 on PIC

#define MTAP_SW_MTAP 0x04
#define MTAP_COMMAND 0x07
#define MCHP_STATUS 0x00
#define MCHP_ERASE 0xFC
#define CFGRDY 3
#define FCBUSY 2

#define KEY_SEQ 0b01001101010000110100100001010000

void configure_io(void);

// Only needed for two-wire mode
void enter_programming_mode(void);

void read_device_status(void);

void set_mode(uint8_t mode);

void send_command(uint8_t cmd);

uint32_t xfer_data(uint32_t data);

void erase_device(void);
