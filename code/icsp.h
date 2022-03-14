#include "rpi.h"

void enter_programming_mode(void);

void read_device_status(void);

void set_mode(uint8_t mode);

void send_command(uint8_t cmd);

void xfer_data(uint32_t status);
