#include <types.h>
#include <serial_core.h>

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

// Read Exception Syndrome Register
static inline uint64_t read_esr_el1(void)
{
    uint64_t val;
    __asm__ volatile("mrs %0, esr_el1" : "=r"(val));
    return val;
}

// Read Exception Link Register (return address)
static inline uint64_t read_elr_el1(void)
{
    uint64_t val;
    __asm__ volatile("mrs %0, elr_el1" : "=r"(val));
    return val;
}

// Read Saved Program Status Register
static inline uint64_t read_spsr_el1(void)
{
    uint64_t val;
    __asm__ volatile("mrs %0, spsr_el1" : "=r"(val));
    return val;
}

// Read Fault Address Register
static inline uint64_t read_far_el1(void)
{
    uint64_t val;
    __asm__ volatile("mrs %0, far_el1" : "=r"(val));
    return val;
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

/**
 * Generic exception handler called from assembly
 * @param type: Exception type (0-15)
 * We move the type value to register x0 before calling this function
 * which corresponds to the first argument in AArch64 calling convention.
 */
void exception_handler_c(uint32_t type)
{
    char hex_buf[19];
    uint64_t esr, elr, spsr, far;
    
    // Read exception information registers
    esr  = read_esr_el1();
    elr  = read_elr_el1();
    spsr = read_spsr_el1();
    far  = read_far_el1();
    
    // Print exception information
    uart_poll_puts("\n");
    uart_poll_puts("======================================\n");
    uart_poll_puts("EXCEPTION OCCURRED!\n");
    uart_poll_puts("======================================\n");
    
    if (type < 16) {
        uart_poll_puts("Type: ");
        uart_poll_puts(exception_names[type]);
        uart_poll_puts("\n");
    }
    
    uart_poll_puts("ESR_EL1:  ");
    uint64_to_hex(esr, hex_buf);
    uart_poll_puts(hex_buf);
    uart_poll_puts("\n");
    
    uart_poll_puts("ELR_EL1:  ");
    uint64_to_hex(elr, hex_buf);
    uart_poll_puts(hex_buf);
    uart_poll_puts("\n");
    
    uart_poll_puts("SPSR_EL1: ");
    uint64_to_hex(spsr, hex_buf);
    uart_poll_puts(hex_buf);
    uart_poll_puts("\n");
    
    uart_poll_puts("FAR_EL1:  ");
    uint64_to_hex(far, hex_buf);
    uart_poll_puts(hex_buf);
    uart_poll_puts("\n");
    
    uart_poll_puts("======================================\n");
    
    // Halt the system
    uart_poll_puts("System halted.\n");
    for (;;) {
        __asm__ volatile("wfe");
    }
}
