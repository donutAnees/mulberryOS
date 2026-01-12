#include <stddef.h>
#include <kernel/irq.h>

/* Global IRQ handler function pointer */
void (*handle_arch_irq)(void) = NULL;

void set_handle_irq(void (*handler)(void))
{
    handle_arch_irq = handler;
}

void irq_handler_c(void)
{
    if (handle_arch_irq)
        handle_arch_irq();
}

