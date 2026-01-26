#include <stddef.h>
#include <kernel/irq_chip.h>

static struct irq_desc irq_desc_table[NR_IRQS];

void irq_init(void)
{
    for (unsigned int i = 0; i < NR_IRQS; i++) {
        irq_desc_table[i].irq_data.hwirq = i;
        irq_desc_table[i].chip = NULL;
        irq_desc_table[i].handle_irq = NULL;
        irq_desc_table[i].action = NULL;
        irq_desc_table[i].irq_count = 0;
        irq_desc_table[i].name = NULL;
    }
}

struct irq_desc *irq_get_desc(unsigned int irq)
{
    if (irq >= NR_IRQS)
        return NULL;
    return &irq_desc_table[irq];
}

int irq_set_chip_and_handler(unsigned int irq, struct irq_chip *chip,
                             irq_flow_handler_t handler)
{
    struct irq_desc *desc = irq_get_desc(irq);
    if (!desc)
        return -1;

    desc->chip = chip;
    desc->handle_irq = handler;
    return 0;
}

int irq_set_chained_handler(unsigned int irq,
                                 irq_flow_handler_t handler)
{
    struct irq_desc *desc = irq_get_desc(irq);
    if (!desc)
        return -1;

    desc->handle_irq = handler;
    return 0;
}

void generic_handle_irq(unsigned int irq)
{
    struct irq_desc *desc = irq_get_desc(irq);

    if (!desc)
        return;

    desc->irq_count++;

    /* Call flow handler if registered */
    if (desc->handle_irq)
        desc->handle_irq(desc);
}

irqreturn_t handle_irq_event(struct irq_desc *desc)
{
    struct irqaction *action = desc->action;
    irqreturn_t ret = IRQ_NONE;

    /* Walk the action chain */
    while (action) {
        irqreturn_t res = action->handler(desc->irq_data.hwirq, action->dev_id);
        
        if (res == IRQ_HANDLED)
            ret = IRQ_HANDLED;
            
        action = action->next;
    }

    return ret;
}

void irq_mask_and_ack(struct irq_desc *d)
{
    if (d->chip && d->chip->irq_mask)
        d->chip->irq_mask(&d->irq_data);
    
    if (d->chip && d->chip->irq_ack)
        d->chip->irq_ack(&d->irq_data);
}

/* Simple flow handler for basic interrupts */
void handle_simple_irq(struct irq_desc *desc)
{
    if (desc->action) {
        handle_irq_event(desc);
    }
}

/* Level handler, the hardware line will stay asserted until the device clears the condition
 * to stop anymore interrupts firing, we will mask the IRQ and then acknowledge it
 * and then unmask it after handling
 */
void handle_level_irq(struct irq_desc *desc)
{
    irq_mask_and_ack(desc);
    if (desc->action) {
        handle_irq_event(desc);
    }
    enable_irq(desc->irq_data.hwirq);
}

/* Helper function to allocate irqaction */
static struct irqaction *alloc_irqaction(void)
{
    /* TODO: Simple static allocation for now, should later be replaced by dynamic alloc */
    static struct irqaction irqaction_pool[NR_IRQS * 2];
    static unsigned int next_action = 0;
    
    if (next_action >= (NR_IRQS * 2))
        return NULL;
        
    return &irqaction_pool[next_action++];
}

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
                void *dev_id)
{
    struct irq_desc *desc = irq_get_desc(irq);
    struct irqaction *action;
    
    if (!desc || !handler)
        return -1;

    /* Allocate new irqaction */
    action = alloc_irqaction();
    if (!action)
        return -1;

    /* Fill in action structure */
    action->handler = handler;
    action->dev_id = dev_id;
    action->flags = flags;
    action->next = NULL;

    /* Add to front of action chain */
    action->next = desc->action;
    desc->action = action;
    return 0;
}

void free_irq(unsigned int irq, void *dev_id)
{
    struct irq_desc *desc = irq_get_desc(irq);
    struct irqaction **action_ptr, *action;
    
    if (!desc)
        return;

    /* Find and remove action with matching dev_id */
    action_ptr = &desc->action;
    while (*action_ptr) {
        action = *action_ptr;
        if (action->dev_id == dev_id) {
            // Eventhough we remove the action from the list
            // It is statically allocated so no freeing
            // TODO: free memory once dynamic allocation is live
            *action_ptr = action->next;
            return;
        }
        action_ptr = &action->next;
    }
}

void enable_irq(unsigned int irq)
{
    struct irq_desc *desc = irq_get_desc(irq);
    
    if (!desc || !desc->chip || !desc->chip->irq_unmask)
        return;

    desc->chip->irq_unmask(&desc->irq_data);
}

void disable_irq(unsigned int irq)
{
    struct irq_desc *desc = irq_get_desc(irq);
    
    if (!desc || !desc->chip || !desc->chip->irq_mask)
        return;

    desc->chip->irq_mask(&desc->irq_data);
}