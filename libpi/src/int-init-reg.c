#include "rpi.h"
#include "rpi-interrupts.h"
#include "vector-base.h"

void int_init_reg (void *int_vector_addr) {
    // put interrupt flags in known state. 
    //  BCM2835 manual, section 7.5
    PUT32(Disable_IRQs_1, 0xffffffff);
    PUT32(Disable_IRQs_2, 0xffffffff);
    dev_barrier();

    /*
     * Copy in interrupt vector table and FIQ handler _table and _table_end
     * are symbols defined in the interrupt assembly file, at the beginning
     * and end of the table and its embedded constants.
     */
    extern unsigned _interrupt_table;
    extern unsigned _interrupt_table_end;

    vector_base_set(int_vector_addr);

    // where the interrupt handlers go.
// #   define RPI_VECTOR_START  0
    unsigned *dst = int_vector_addr,
                 *src = &_interrupt_table,
                 n = &_interrupt_table_end - src;
    for(int i = 0; i < n; i++)
        dst[i] = src[i];
}
