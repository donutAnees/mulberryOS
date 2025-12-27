#include <types.h>
#include <serial_core.h>

extern void pl011_register(void);

void kernel_main(void)
{
    // TODO: Dynamic registration should be done, rather than calling directly
    pl011_register();

    uart_poll_puts("\n");
    uart_poll_puts("MulberryOS booting\n");
    uart_poll_puts("PL011 UART initialized\n");

    /* Main idle loop */
    for (;;) {
        __asm__ volatile("wfe");
    }
}
