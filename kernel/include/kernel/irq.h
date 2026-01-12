#ifndef _KERNEL_IRQ_H
#define _KERNEL_IRQ_H

/*
 * set_handle_irq - Set the global IRQ handler
 * @handler: Function to call from low-level IRQ code
 */
void set_handle_irq(void (*handler)(void));

/*
 * irq_handler_c - C-level IRQ handler called from assembly
 */
void irq_handler_c(void);

#endif /* _KERNEL_IRQ_H */