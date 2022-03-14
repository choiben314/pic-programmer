#include "rpi.h"

#define KEY_SEQ 0b01001101010000110100100001010000

void configure_io(void);

// Only needed for two-wire mode
void enter_programming_mode(void);

void read_device_status(void);

void set_mode(uint8_t mode);

void send_command(uint8_t cmd);

void xfer_data(uint32_t status);
