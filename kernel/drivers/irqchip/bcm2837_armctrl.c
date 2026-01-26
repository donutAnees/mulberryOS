/*
 * BCM2837 Global (ARMCTRL) Interrupt Controller
 *
 * This driver implements support for the Broadcom BCM2837 ARMCTRL
 * interrupt controller, which aggregates interrupts from GPU and
 * SoC peripherals (UART, SPI, I2C, USB, etc.) into three interrupt
 * banks:
 *
 *   - Basic pending (ARM/local-related sources)
 *   - Pending 1 (GPU peripheral interrupts 0–31)
 *   - Pending 2 (GPU peripheral interrupts 32–63)
 *
 * The ARMCTRL controller does not deliver interrupts directly to the
 * CPU cores. Instead, it asserts one of the GPU interrupt output lines
 * which is connected to the per-CPU local interrupt controller.
 *
 * The local interrupt controller is the only interrupt source directly
 * visible to the CPU. It acts as the parent interrupt controller and
 * raises a single parent IRQ when any ARMCTRL interrupt is pending.
 *
 */

#include <stdint.h>
#include <kernel/irq_chip.h>

#define HWIRQ_BANK(i)       (i >> 5)
#define HWIRQ_BIT(i)        (1U << (i & 0x1f))

#define NR_BANKS        3
#define IRQ_PER_BANK    32

/* ARM Control interrupt controller base address (0x7e00b200 in VC bus address) */
#define ARMCTRL_IRQ_BASE    0x3F00B200
#define LOCAL_IRQ_GPU_FAST  8

static const int reg_pending[] = { 0x00, 0x04, 0x08 };
static const int reg_enable[]  = { 0x18, 0x10, 0x14 };
static const int reg_disable[] = { 0x24, 0x1c, 0x20 };

static const int bank_irqs[] = { 8, 32, 32 };

struct bcm2837_armctrl_intc {
	uintptr_t base;
	volatile uint32_t *pending[NR_BANKS];
	volatile uint32_t *enable[NR_BANKS];
	volatile uint32_t *disable[NR_BANKS];
};

static struct bcm2837_armctrl_intc intc;

static void bcm2837_armctrl_mask_irq(struct irq_data *d)
{
	*intc.disable[HWIRQ_BANK(d->hwirq)] = HWIRQ_BIT(d->hwirq);
}

static void bcm2837_armctrl_unmask_irq(struct irq_data *d)
{
	*intc.enable[HWIRQ_BANK(d->hwirq)] = HWIRQ_BIT(d->hwirq);
}


static struct irq_chip armctrl_chip = {
    .name       = "armctrl-chip",
    .irq_mask   = bcm2837_armctrl_mask_irq,
    .irq_unmask = bcm2837_armctrl_unmask_irq,
};

static void bcm2837_chained_armctrl_irq(struct irq_desc *desc)
{
    for (int b = 0; b < NR_BANKS; b++) {
        uint32_t pending = *intc.pending[b];
        while (pending) {
            int bit = __builtin_ffs(pending) - 1;
            unsigned int hwirq = (b << 5) | bit; // b * 32 + bit
            unsigned int virq = hwirq + ARMCTRL_IRQ_OFFSET;
            generic_handle_irq(virq);
            pending &= ~(1U << bit);
        }
    }
}

int bcm2837_armctrl_init(void)
{
    intc.base = ARMCTRL_IRQ_BASE;
    for (int b = 0; b < NR_BANKS; b++) {
        intc.pending[b] = (volatile uint32_t *)(intc.base + reg_pending[b]);
        intc.enable[b] = (volatile uint32_t *)(intc.base + reg_enable[b]);
        intc.disable[b] = (volatile uint32_t *)(intc.base + reg_disable[b]);
        
        for (int i = 0; i < bank_irqs[b]; i++) {
            unsigned int hwirq = ((b << 5) | i); // b * 32 + i
            unsigned int virq = hwirq + ARMCTRL_IRQ_OFFSET;
            irq_set_chip_and_handler(virq, &armctrl_chip, handle_level_irq);
        }
    }
    
    // Set chained handler for parent IRQ 
    irq_set_chained_handler(LOCAL_IRQ_GPU_FAST, bcm2837_chained_armctrl_irq);
    return 0;
}
