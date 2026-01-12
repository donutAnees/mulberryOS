#include <stddef.h>
#include <kernel/irq_chip.h>

static struct irq_desc irq_desc_table[NR_IRQS];

void irq_init(void)
{
    for (unsigned int i = 0; i < NR_IRQS; i++) {
        irq_desc_table[i].irq_data.hwirq = i;
        irq_desc_table[i].chip = NULL;
        irq_desc_table[i].handler = NULL;
        irq_desc_table[i].handler_data = NULL;
    }
}

struct irq_desc *irq_get_desc(unsigned int irq)
{
    if (irq >= NR_IRQS)
        return NULL;
    return &irq_desc_table[irq];
}

int irq_set_chip_and_handler(unsigned int irq, struct irq_chip *chip,
                             irq_handler_t handler)
{
    struct irq_desc *desc = irq_get_desc(irq);
    if (!desc)
        return -1;

    desc->chip = chip;
    desc->handler = handler;
    return 0;
}

void generic_handle_irq(unsigned int irq)
{
    struct irq_desc *desc = irq_get_desc(irq);

    if (!desc)
        return;

    /* Call handler if registered */
    if (desc->handler)
        desc->handler(irq, desc->handler_data);
}