#include "icsp.h"



// Enter enhanced ICSP

// MCLR pin driven high, then low
// 32-bit key sequence is clocked into PGDx
// MCLR driven high with specific period of time and held




// Read device status

// SetMode (6'b011111) to force TAP controller into Run Test/Idle state

// SendCommand(MTAP_SW_MTAP)
// SendCommand(MTAP_COMMAND)
// statusVal = XferData(MCHP_STATUS)
// If CFGRDY (statusVal<3>) is 0 and FCBUSY (statusVal<2>) is 1
    // delay 10ms
    // read Xfer data again
// Done.