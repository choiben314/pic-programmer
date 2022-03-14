#include "rpi.h"
#include "breakpoint.h"
#include "armv6-debug-impl.h"

void brkpt_mismatch_start(void) {
    if (!cp14_is_enabled()) {
        cp14_enable();
    }
    cp14_bcr0_enable();
    brkpt_mismatch_set(0xFFFFFFF0);
}

// stop mismatching.
void brkpt_mismatch_stop(void) {
    if (!cp14_is_enabled()) {
        cp14_enable();
    }
    cp14_bcr0_disable();
}

// set a mismatch on <addr> --- call the prefetch abort handler on mismatch.
//  - you cannot get mismatches in "privileged" modes (all modes other than
//    USER_MODE)
//  - once you are in USER_MODE you cannot switch modes on your own since the 
//    the required "msr" instruction will be ignored.  if you do want to 
//    return from user space you'd have to do a system call ("swi") that switches.
void brkpt_mismatch_set(uint32_t addr) {
    
    if (!cp14_is_enabled()) {
        cp14_enable();
    }
    
    uint32_t b = 0;

    cp14_bvr0_set(addr);

    b = bits_set(b, 0, 8, 0b111100111);
    b = bits_set(b, 21, 22, 0b10);

    cp14_bcr0_set(b);

    prefetch_flush();

    assert(cp14_bcr0_is_enabled());
}