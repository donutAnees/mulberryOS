#ifndef _AARCH64_SYSREG_H
#define _AARCH64_SYSREG_H

#include <types.h>

/*
 * System Register Read/Write Helpers
 */

#define read_sysreg(reg) ({					\
	uint64_t __val;						\
	__asm__ volatile("mrs %0, " #reg : "=r" (__val));	\
	__val;							\
})

#define write_sysreg(val, reg) do {				\
	uint64_t __val = (uint64_t)(val);			\
	__asm__ volatile("msr " #reg ", %0" : : "r" (__val));	\
} while (0)

/*
 * Exception Level Registers
 */

static inline uint64_t read_esr_el1(void)
{
	return read_sysreg(esr_el1);
}

static inline uint64_t read_elr_el1(void)
{
	return read_sysreg(elr_el1);
}

static inline uint64_t read_spsr_el1(void)
{
	return read_sysreg(spsr_el1);
}

static inline uint64_t read_far_el1(void)
{
	return read_sysreg(far_el1);
}

static inline uint64_t read_vbar_el1(void)
{
	return read_sysreg(vbar_el1);
}

static inline void write_vbar_el1(uint64_t val)
{
	write_sysreg(val, vbar_el1);
}

static inline uint64_t read_currentel(void)
{
	return read_sysreg(CurrentEL);
}

#endif /* _AARCH64_SYSREG_H */
