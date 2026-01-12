#ifndef _KERNEL_IRQ_CHIP_H
#define _KERNEL_IRQ_CHIP_H

/* 
* irq_data represents one interrupt line in the system.
* for now it only contains the hardware IRQ number.
*/
struct irq_data {
    unsigned int hwirq;     /* hardware IRQ number */
};


/*
* irq_chip defines the operations for an interrupt controller chip.
* an interrupt controller can have multiple irq_chips to handle
* different types of interrupts.
* @name: Name of the irq_chip.
* @irq_mask: Function to mask (disable) the interrupt.
* @irq_unmask: Function to unmask (enable) the interrupt.
*/
struct irq_chip {
    const char *name;
    void (*irq_mask)(struct irq_data *d);
    void (*irq_unmask)(struct irq_data *d);
};

/*
 * IRQ handler function type
 * @irq: The IRQ number
 * @data: Private data passed during registration
 */
typedef void (*irq_handler_t)(unsigned int irq, void *data);

/*
 * irq_desc describes a single IRQ line, linking together
 * the chip, handler, and any private data.
 */
struct irq_desc {
    struct irq_data         irq_data;       /* IRQ data (hwirq, etc.) */
    struct irq_chip         *chip;          /* IRQ chip for this IRQ */
    irq_handler_t           handler;        /* High-level handler */
    void                    *handler_data;  /* Data passed to handler */
};

/* Maximum number of IRQs supported */
#define NR_IRQS 12

/*
 * irq_set_chip_and_handler - Set both chip and handler for an IRQ
 * @irq: IRQ number
 * @chip: Pointer to irq_chip structure
 * @handler: Handler function
 * function to set both chip and handler at once.
 */
int irq_set_chip_and_handler(unsigned int irq, struct irq_chip *chip,
                             irq_handler_t handler);

/*
 * irq_get_desc - Get the IRQ descriptor for an IRQ number
 * @irq: IRQ number
 * Returns pointer to irq_desc. 
 */
struct irq_desc *irq_get_desc(unsigned int irq);

/*
 * generic_handle_irq - Call the handler for an IRQ
 * @irq: IRQ number
 *
 * Called by the low-level IRQ code to dispatch to the registered handler.
 */
void generic_handle_irq(unsigned int irq);

/*
 * irq_init - Initialize the IRQ subsystem
 */
void irq_init(void);

#endif 
