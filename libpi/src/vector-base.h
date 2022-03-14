#ifndef __VECTOR_BASE_SET_H__
#define __VECTOR_BASE_SET_H__
#include "libc/bit-support.h"
#include "asm-helpers.h"

/*
 * vector base address register:
 *   3-121 --- let's us control where the exception jump table is!
 *
 * defines: 
 *  - vector_base_set  
 *  - vector_base_get
 */

static inline void *vector_base_get(void) {
    void *vector_base;
    asm ( "mrc p15, 0, %0, c12, c0, 0" : "=r" (vector_base) );
    return vector_base;
}

// set the vector register to point to <vector_base>.
// must: 
//    - check that it satisfies the alignment restriction.
static inline void vector_base_set(void *vector_base) {
    asm ( "mcr p15, 0, %0, c12, c0, 0" : : "r" (vector_base) );
    void * temp = vector_base_get();
    assert(((unsigned)vector_base_get() & 0b11111) == 0);
    assert(vector_base_get() == vector_base);
}
#endif
