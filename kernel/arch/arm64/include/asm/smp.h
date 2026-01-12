#ifndef _ASM_SMP_H
#define _ASM_SMP_H

#include <asm/sysreg.h>

/*
 * For Raspberry Pi Zero 2 W (BCM2837), the CPU ID is in Aff0 (bits [1:0])
 * The quad-core Cortex-A53 uses simple linear CPU numbering 0-3
 */

/**
 * smp_processor_id - Get the current CPU/core ID
 *
 * Returns the hardware CPU ID.
 */
static inline unsigned int smp_processor_id(void)
{
    uint64_t mpidr = read_sysreg(mpidr_el1);
    /*
     * On BCM2837, CPU ID is lowest 2 bits of MPIDR
     */
    return (unsigned int)(mpidr & 0x3);
}

#endif