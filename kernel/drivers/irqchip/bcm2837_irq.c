#include <stdint.h>
#include <stddef.h>
#include <kernel/irq_chip.h>
#include <kernel/irq.h>
#include <asm/smp.h>

#define LOCAL_IRQ_CNTPSIRQ	0
#define LOCAL_IRQ_CNTPNSIRQ	1
#define LOCAL_IRQ_CNTHPIRQ	2
#define LOCAL_IRQ_CNTVIRQ	3
#define LOCAL_IRQ_MAILBOX0	4
#define LOCAL_IRQ_MAILBOX1	5
#define LOCAL_IRQ_MAILBOX2	6
#define LOCAL_IRQ_MAILBOX3	7
#define LOCAL_IRQ_GPU_FAST	8
#define LOCAL_IRQ_PMU_FAST	9
#define LOCAL_IRQ_SIZE LOCAL_IRQ_PMU_FAST + 1

// Local Timer base address
// Last 4 bits -> IRQ enable
// Next 4 bits -> FIQ enable
#define LOCAL_TIMER_BASE_CONTROL_OFFSET 0x040

/* IRQ pending register per CPU */
#define LOCAL_IRQ_PENDING_OFFSET(cpu)   (0x060 + (cpu * 4))

/* Setting bits 0-3 enables PMU interrupts, each corresponding to a CPU */
#define LOCAL_PM_ROUTING_SET		0x010
/* Setting bits 0-3 disables PMU interrupts, each corresponding to a CPUß */
#define LOCAL_PM_ROUTING_CLR		0x014

/* TODO: not a fan of defining these addresses hardcoded, will DT once we have that setup */
#define BCM2837_IRQ_BASE 0x40000000

struct bcm2837_irqchip_intc {
   uintptr_t base; 
};

static struct bcm2837_irqchip_intc bcm2837_irqchip = {
    .base = BCM2837_IRQ_BASE,
};

static void bcm2837_timer_irq_mask(struct irq_data *d)
{
    int cpu = smp_processor_id();

    /*
     * Convert hardware IRQ number to bit index:
     * Right now since we start with 0 as hwirq 
     * and that maps to the timer this doesnt matter
     *   hwirq 0 → bit 0 (CNTPSIRQ)
     *   hwirq 1 → bit 1 (CNTPNSIRQ)
     *   hwirq 2 → bit 2 (CNTHPIRQ)
     *   hwirq 3 → bit 3 (CNTVIRQ)
     */
    unsigned int bit = d->hwirq - LOCAL_IRQ_CNTPSIRQ;

    /*
     * Per-CPU timer control register:
     *   base + offset + (cpu * 4)
     */
    volatile uint32_t *reg =
        (volatile uint32_t *)(bcm2837_irqchip.base +
                              LOCAL_TIMER_BASE_CONTROL_OFFSET +
                              (cpu * 4));
    *reg &= ~(1U << bit);
}

static void bcm2837_timer_irq_unmask(struct irq_data *d)
{
    int cpu = smp_processor_id();

    /*
     * Convert hardware IRQ number to register bit index
     */
    unsigned int bit = d->hwirq - LOCAL_IRQ_CNTPSIRQ;

    /*
     * Per-CPU timer control register
     */
    volatile uint32_t *reg =
        (volatile uint32_t *)(bcm2837_irqchip.base +
                              LOCAL_TIMER_BASE_CONTROL_OFFSET +
                              (cpu * 4));

    *reg |= (1U << bit);
}


static struct irq_chip bcm2837_timer_irqchip = {
    .name       = "bcm2837-timer-irqchip",
    .irq_mask   = bcm2837_timer_irq_mask,
    .irq_unmask = bcm2837_timer_irq_unmask,
};

static void bcm2837_pmu_irq_mask(struct irq_data *d)
{
    volatile uint32_t *addr = (volatile uint32_t *)(bcm2837_irqchip.base + LOCAL_PM_ROUTING_CLR);
    *addr = (1U << smp_processor_id());
}

static void bcm2837_pmu_irq_unmask(struct irq_data *d)
{
    volatile uint32_t *addr = (volatile uint32_t *)(bcm2837_irqchip.base + LOCAL_PM_ROUTING_SET);
    *addr = (1U << smp_processor_id());
}

static struct irq_chip bcm2837_pmu_irqchip = {
    .name       = "bcm2837-pmu-irqchip",
    .irq_mask   = bcm2837_pmu_irq_mask,
    .irq_unmask = bcm2837_pmu_irq_unmask,
};

static void bcm2837_gpu_irq_mask(struct irq_data *d)
{
}

static void bcm2837_gpu_irq_unmask(struct irq_data *d)
{
}

static struct irq_chip bcm2837_gpu_irqchip = {
    .name       = "bcm2837-gpu-irqchip",
    .irq_mask   = bcm2837_gpu_irq_mask,
    .irq_unmask = bcm2837_gpu_irq_unmask,
};


/*
 * bcm2836_arm_irqchip_handle_irq - Main IRQ dispatcher
 * 
 * This function is called from the low-level IRQ exception handler.
 * It reads the local interrupt pending register and dispatches to
 * the appropriate handler via generic_handle_irq().
 */
static void bcm2836_arm_irqchip_handle_irq(void)
{
    int cpu = smp_processor_id();
    volatile uint32_t *pending_reg;
    uint32_t pending;
    int irq;

    /* Read local IRQ pending register for this CPU */
    pending_reg = (volatile uint32_t *)(bcm2837_irqchip.base + 
                                       LOCAL_IRQ_PENDING_OFFSET(cpu));
    pending = *pending_reg;

    if (!pending)
        return; /* Spurious interrupt */

    /* Find first set bit (lowest IRQ number) */
    irq = __builtin_ffs(pending) - 1;
    if (irq >= 0 && irq < LOCAL_IRQ_SIZE)
        generic_handle_irq(irq);
}

int bcm2837_irq_init(void)
{
    // TODO: Ideally we should be assigning a virtual IRQ number for each hardware IRQ
    // Linux does this using irq_domain, we can implement something similar later
    
    // Register timer IRQs
    for (unsigned int i = LOCAL_IRQ_CNTPSIRQ; i <= LOCAL_IRQ_CNTVIRQ; i++) {
        irq_set_chip_and_handler(i, &bcm2837_timer_irqchip, NULL);
    }
    // Register PMU IRQ
    irq_set_chip_and_handler(LOCAL_IRQ_PMU_FAST, &bcm2837_pmu_irqchip, NULL);
    // Register GPU IRQ
    irq_set_chip_and_handler(LOCAL_IRQ_GPU_FAST, &bcm2837_gpu_irqchip, NULL);

    // Set this as the main IRQ handler
    set_handle_irq(bcm2836_arm_irqchip_handle_irq);
    
    return 0;
}