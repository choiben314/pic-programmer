#include "rpi.h"
#include "jtag.h"

void notmain(void) {
    configure_io();
    read_device_status();
}