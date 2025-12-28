#include <types.h>
#include <serial_core.h>
#include <exception.h>
#include <asm/sysreg.h>
#include <asm/esr.h>

static const char *exception_names[] = {
    "Current EL with SP_EL0 - Sync",
    "Current EL with SP_EL0 - IRQ",
    "Current EL with SP_EL0 - FIQ",
    "Current EL with SP_EL0 - SError",
    "Current EL with SP_ELx - Sync",
    "Current EL with SP_ELx - IRQ",
    "Current EL with SP_ELx - FIQ",
    "Current EL with SP_ELx - SError",
    "Lower EL (AArch64) - Sync",
    "Lower EL (AArch64) - IRQ",
    "Lower EL (AArch64) - FIQ",
    "Lower EL (AArch64) - SError",
    "Lower EL (AArch32) - Sync",
    "Lower EL (AArch32) - IRQ",
    "Lower EL (AArch32) - FIQ",
    "Lower EL (AArch32) - SError",
};

/*
 * ESR Exception Class strings
 */
static const char *esr_class_str[] = {
	[0 ... ESR_ELx_EC_MAX]		= "UNRECOGNIZED EC",
	[ESR_ELx_EC_UNKNOWN]		= "Unknown/Uncategorized",
	[ESR_ELx_EC_WFx]		    = "WFI/WFE",
	[ESR_ELx_EC_CP15_32]		= "CP15 MCR/MRC",
	[ESR_ELx_EC_CP15_64]		= "CP15 MCRR/MRRC",
	[ESR_ELx_EC_CP14_MR]		= "CP14 MCR/MRC",
	[ESR_ELx_EC_CP14_LS]		= "CP14 LDC/STC",
	[ESR_ELx_EC_FP_ASIMD]		= "ASIMD",
	[ESR_ELx_EC_CP10_ID]		= "CP10 MRC/VMRS",
	[ESR_ELx_EC_PAC]		    = "PAC",
	[ESR_ELx_EC_CP14_64]		= "CP14 MCRR/MRRC",
	[ESR_ELx_EC_BTI]		    = "BTI",
	[ESR_ELx_EC_ILL]		    = "PSTATE.IL",
	[ESR_ELx_EC_SVC32]		    = "SVC (AArch32)",
	[ESR_ELx_EC_HVC32]		    = "HVC (AArch32)",
	[ESR_ELx_EC_SMC32]		    = "SMC (AArch32)",
	[ESR_ELx_EC_SVC64]		    = "SVC (AArch64)",
	[ESR_ELx_EC_HVC64]		    = "HVC (AArch64)",
	[ESR_ELx_EC_SMC64]		    = "SMC (AArch64)",
	[ESR_ELx_EC_SYS64]		    = "MSR/MRS (AArch64)",
	[ESR_ELx_EC_SVE]		    = "SVE",
	[ESR_ELx_EC_ERET]		    = "ERET/ERETAA/ERETAB",
	[ESR_ELx_EC_FPAC]		    = "FPAC",
	[ESR_ELx_EC_SME]		    = "SME",
	[ESR_ELx_EC_IMP_DEF]		= "EL3 IMP DEF",
	[ESR_ELx_EC_IABT_LOW]		= "IABT (lower EL)",
	[ESR_ELx_EC_IABT_CUR]       = "IABT (current EL)",
	[ESR_ELx_EC_PC_ALIGN]		= "PC Alignment",
	[ESR_ELx_EC_DABT_LOW]		= "DABT (lower EL)",
	[ESR_ELx_EC_DABT_CUR]		= "DABT (current EL)",
	[ESR_ELx_EC_SP_ALIGN]		= "SP Alignment",
	[ESR_ELx_EC_MOPS]		    = "MOPS",
	[ESR_ELx_EC_FP_EXC32]		= "FP (AArch32)",
	[ESR_ELx_EC_FP_EXC64]		= "FP (AArch64)",
	[ESR_ELx_EC_GCS]		    = "Guarded Control Stack",
	[ESR_ELx_EC_SERROR]		    = "SError",
	[ESR_ELx_EC_BREAKPT_LOW]	= "Breakpoint (lower EL)",
	[ESR_ELx_EC_BREAKPT_CUR]	= "Breakpoint (current EL)",
	[ESR_ELx_EC_SOFTSTP_LOW]	= "Software Step (lower EL)",
	[ESR_ELx_EC_SOFTSTP_CUR]	= "Software Step (current EL)",
	[ESR_ELx_EC_WATCHPT_LOW]	= "Watchpoint (lower EL)",
	[ESR_ELx_EC_WATCHPT_CUR]	= "Watchpoint (current EL)",
	[ESR_ELx_EC_BKPT32]		    = "BKPT (AArch32)",
	[ESR_ELx_EC_VECTOR32]		= "Vector catch (AArch32)",
	[ESR_ELx_EC_BRK64]		    = "BRK (AArch64)",
};

/*
 * Get exception class string from ESR
 */
const char *esr_get_class_string(uint64_t esr)
{
    return esr_class_str[ESR_ELx_EC(esr)];
}

/*
 * Get exception type string
 */
const char *exception_type_string(uint32_t type)
{
    if (type < 16)
        return exception_names[type];
    return "Unknown exception type";
}

// Simple hex to string conversion
static void uint64_to_hex(uint64_t value, char *buf)
{
    const char hex_chars[] = "0123456789ABCDEF";
    int i;
    
    buf[0] = '0';
    buf[1] = 'x';
    
    for (i = 0; i < 16; i++) {
        buf[2 + i] = hex_chars[(value >> (60 - i * 4)) & 0xF];
    }
    buf[18] = '\0';
}

/*
 * Dump detailed exception information
 */
void dump_exception_info(uint32_t type, uint64_t esr, uint64_t elr,
                         uint64_t spsr, uint64_t far)
{
    char hex_buf[19];
    uint32_t ec = ESR_ELx_EC(esr);
    
    uart_poll_puts("\n");
    uart_poll_puts("======================================\n");
    uart_poll_puts("EXCEPTION OCCURRED!\n");
    uart_poll_puts("======================================\n");
    
    /* Exception vector entry */
    uart_poll_puts("Vector: ");
    uart_poll_puts(exception_type_string(type));
    uart_poll_puts("\n");
    
    /* Exception class from ESR */
    uart_poll_puts("Class:  ");
    uart_poll_puts(esr_get_class_string(esr));
    uart_poll_puts(" (EC=0x");
    /* Print EC in hex (2 digits) */
    hex_buf[0] = "0123456789ABCDEF"[(ec >> 4) & 0xF];
    hex_buf[1] = "0123456789ABCDEF"[ec & 0xF];
    hex_buf[2] = ')';
    hex_buf[3] = '\0';
    uart_poll_puts(hex_buf);
    uart_poll_puts("\n");
    
    uart_poll_puts("ESR_EL1:  ");
    uint64_to_hex(esr, hex_buf);
    uart_poll_puts(hex_buf);
    uart_poll_puts("\n");
    
    uart_poll_puts("ELR_EL1:  ");
    uint64_to_hex(elr, hex_buf);
    uart_poll_puts(hex_buf);
    uart_poll_puts(" (PC at exception)\n");
    
    uart_poll_puts("SPSR_EL1: ");
    uint64_to_hex(spsr, hex_buf);
    uart_poll_puts(hex_buf);
    uart_poll_puts("\n");
    
    uart_poll_puts("FAR_EL1:  ");
    uint64_to_hex(far, hex_buf);
    uart_poll_puts(hex_buf);
    uart_poll_puts(" (Fault address)\n");
    
    uart_poll_puts("======================================\n");
}

/**
 * Generic exception handler called from assembly
 * @param type: Exception type (0-15)
 * We move the type value to register x0 before calling this function
 * which corresponds to the first argument in AArch64 calling convention.
 */
void exception_handler_c(uint32_t type)
{
    uint64_t esr, elr, spsr, far;
    
    /* Read exception information registers */
    esr  = read_esr_el1();
    elr  = read_elr_el1();
    spsr = read_spsr_el1();
    far  = read_far_el1();
    
    /* Display exception information */
    dump_exception_info(type, esr, elr, spsr, far);
    
    /* Halt the system */
    uart_poll_puts("System halted.\n");
    for (;;) {
        __asm__ volatile("wfe");
    }
}
