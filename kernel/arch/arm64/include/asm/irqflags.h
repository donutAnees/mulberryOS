#ifndef _ASM_IRQFLAGS_H
#define _ASM_IRQFLAGS_H

/*
 * AArch64 DAIF (Debug/SError/IRQ/FIQ mask) helpers.
 *
 * DAIF register bits:
 *   Bit 3 (D) - Debug exceptions mask
 *   Bit 2 (A) - SError (asynchronous abort) mask
 *   Bit 1 (I) - IRQ mask
 *   Bit 0 (F) - FIQ mask
 *
 * daifset sets (masks) the specified bits.
 * daifclr clears (unmasks) the specified bits.
 * The immediate is a 4-bit bitmask over {F (#1), I (#2), A (#4), D (#8)}.
 */

/* IRQ enable/disable (bit 1 = I) */
static inline void local_irq_enable(void)
{
    asm volatile("msr daifclr, #2" : : : "memory");
}

static inline void local_irq_disable(void)
{
    asm volatile("msr daifset, #2" : : : "memory");
}

/* Save DAIF and disable IRQs — for critical sections */
static inline unsigned long local_irq_save(void)
{
    unsigned long flags;
    asm volatile(
        "mrs    %0, daif\n"
        "msr    daifset, #2"
        : "=r" (flags)
        :
        : "memory"
    );
    return flags;
}

/* Restore DAIF from saved flags */
static inline void local_irq_restore(unsigned long flags)
{
    asm volatile("msr daif, %0" : : "r" (flags) : "memory");
}

#endif /* _ASM_IRQFLAGS_H */
