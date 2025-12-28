#include <types.h>
#include <serial_core.h>

extern void pl011_register(void);
extern void install_exception_vectors(void);

static inline unsigned int current_el(void)
{
    unsigned long el;
    asm volatile("mrs %0, CurrentEL" : "=r"(el));
    return (el >> 2) & 0x3;
}

void kernel_main(void)
{
    unsigned int el;
    
    // TODO: Dynamic detection of UART base address via DTB parsing
    pl011_register();

    uart_poll_puts("\n");
    uart_poll_puts("MulberryOS booting\n");
    
    // Display exception level
    el = current_el();
    uart_poll_puts("Exception Level: EL");
    uart_poll_putc('0' + el);
    uart_poll_puts("\n");
    
    // Install exception vector table
    uart_poll_puts("Installing exception vectors...\n");
    install_exception_vectors();
    uart_poll_puts("Exception vectors installed at VBAR_EL1\n");

    uart_poll_puts("\nKernel initialization complete.\n");
    uart_poll_puts("Entering idle loop...\n");
    uart_poll_puts("========================================\n");

    /* Main idle loop */
    for (;;) {
        __asm__ volatile("wfe");
    }
}
