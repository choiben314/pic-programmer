#include "rpi.h"
#include "icsp.h"

void notmain(void) {
    configure_io();
    read_device_status();
    // erase_device();
    // enter_serial_execution_mode();
}