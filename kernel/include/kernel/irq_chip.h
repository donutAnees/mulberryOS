#ifndef _KERNEL_IRQ_CHIP_H
#define _KERNEL_IRQ_CHIP_H

/* Forward declarations */
struct irq_desc;

/*
 * IRQ return codes - returned by interrupt handlers
 */
typedef enum irqreturn {
    IRQ_NONE        = 0,    /* Interrupt was not from this device */
    IRQ_HANDLED     = 1,    /* Interrupt was handled successfully */
} irqreturn_t;

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
    void (*irq_ack)(struct irq_data *d);
};

/*
 * IRQ handler function type
 * @irq: The IRQ number
 * @data: Private data passed during registration
 * Returns: IRQ_HANDLED if interrupt was from this device, IRQ_NONE otherwise
 */
typedef irqreturn_t (*irq_handler_t)(unsigned int irq, void *data);

/*
 * IRQ flow handler function type
 * Flow handlers implement the policy for handling different types of interrupts
 */
typedef void (*irq_flow_handler_t)(struct irq_desc *desc);

/* IRQ flags for request_irq() */
#define IRQF_SHARED     1  /* IRQ is shared across multiple devices */
#define IRQF_TIMER      2  /* Timer interrupt */
#define IRQF_PERCPU     3  /* Per-CPU interrupt */

/*
 * IRQaction - represents a registered interrupt handler
 * This allows multiple handlers per IRQ (shared interrupts)
 */
struct irqaction {
    irq_handler_t           handler;        /* Device interrupt handler */
    void                    *dev_id;        /* Device identifier */
    struct irqaction        *next;          /* Next handler in chain */
    unsigned long           flags;          /* IRQ flags (IRQF_SHARED, etc.) */
};

/*
 * irq_desc describes a single IRQ line, linking together
 * the chip, flow handler, and device action chain.
 */
struct irq_desc {
    struct irq_data         irq_data;       /* IRQ data (hwirq, etc.) */
    struct irq_chip         *chip;          /* IRQ chip for this IRQ */
    irq_flow_handler_t      handle_irq;     /* Flow handler (policy) */
    struct irqaction        *action;        /* Device handler chain */
    unsigned int            irq_count;      /* Total interrupt count */
    const char              *name;          /* IRQ name */
};

/* Maximum number of IRQs supported */
#define NR_IRQS 128

/*
 * HACK: Simple IRQ offset for ARMCTRL controller to avoid collision
 * with local interrupt controller IRQs (0-9).
 *
 * TODO: This is not the right way to handle multiple interrupt controllers.
 * We should implement proper irq_domain mapping (like Linux) where each
 * interrupt controller has its own domain that translates hardware IRQ
 * numbers (hwirq) to system-wide virtual IRQ numbers (virq).
 */
#define ARMCTRL_IRQ_OFFSET  16

/*
 * Flow handlers - implement different interrupt handling policies
 */
void handle_simple_irq(struct irq_desc *desc);
void handle_level_irq(struct irq_desc *desc);

/*
 * IRQ management functions
 */
int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
                void *dev_id);
void free_irq(unsigned int irq, void *dev_id);

/*
 * IRQ enable/disable functions
 */
void enable_irq(unsigned int irq);
void disable_irq(unsigned int irq);

/*
 * IRQ mask and acknowledge interrupt
 */
void irq_mask_and_ack(struct irq_desc *d);

/*
 * irq_set_chip_and_handler - Set both chip and handler for an IRQ
 * @irq: IRQ number
 * @chip: Pointer to irq_chip structure
 * @handler: Flow handler function
 */
int irq_set_chip_and_handler(unsigned int irq, struct irq_chip *chip,
                             irq_flow_handler_t handler);

/*
 * irq_set_chained_handler - Set a chained flow handler for an IRQ
 * @irq: IRQ number
 * @handler: Chained flow handler function
 */                             
int irq_set_chained_handler(unsigned int irq,
                                 irq_flow_handler_t handler);

/*
 * irq_get_desc - Get the IRQ descriptor for an IRQ number
 * @irq: IRQ number
 * Returns pointer to irq_desc. 
 */
struct irq_desc *irq_get_desc(unsigned int irq);

/*
 * generic_handle_irq - Call the flow handler for an IRQ
 * @irq: IRQ number
 *
 * Called by the low-level IRQ code to dispatch to the flow handler.
 */
void generic_handle_irq(unsigned int irq);

/*
 * handle_irq_event - Process the IRQ action chain
 * @desc: IRQ descriptor
 * Returns: IRQ_HANDLED if any handler handled the IRQ, IRQ_NONE otherwise
 */
irqreturn_t handle_irq_event(struct irq_desc *desc);

/*
 * irq_init - Initialize the IRQ subsystem
 */
void irq_init(void);

#endif 
