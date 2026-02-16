#include <stddef.h>
#include <stdint.h>
#include <kernel/clockchip.h>
#include <kernel/irq_chip.h>
#include <container_of.h>
#include <serial_core.h>

/*
 * BCM2837 System Timer
 *
 * We use channel 3 for the kernel timer interrupt.
 *
 * Register Layout (at physical address 0x3F003000):
 *   0x00 - Control/Status register (CS)
 *   0x04 - Counter Lower 32 bits (CLO)
 *   0x08 - Counter Higher 32 bits (CHI)
 *   0x0C - Compare 0 (C0)
 *   0x10 - Compare 1 (C1)
 *   0x14 - Compare 2 (C2)
 *   0x18 - Compare 3 (C3)
 *
 * The CS register bits 0-3 are match flags for each compare channel.
 * Writing 1 to a bit clears the match flag.
 *
 * IRQ: System Timer 3 is connected to GPU IRQ 3 (in the ARMCTRL controller),
 *      which maps to virtual IRQ = 3 + 32 (bank 1 offset) + 16 (ARMCTRL offset) = 51
 */

#define BCM2837_TIMER_BASE 0x3F003000 

/* Register offsets */
#define REG_CONTROL 0x00
#define REG_COUNTER_LOW 0x04
#define REG_COUNTER_HIGH 0x08
#define REG_COMPARE(n) (0x0c + (n) * 4)

/* Timer configuration */
#define DEFAULT_TIMER 3
#define TIMER_MATCH_MASK (1U << DEFAULT_TIMER)

/* Timer frequency: 1MHz */
#define TIMER_FREQ_HZ 1000000

/* IRQ number for System Timer 3 */
/* Bank 1 bit 3 = hwirq 35, + ARMCTRL_IRQ_OFFSET(16) = 51 */
#define SYSTEM_TIMER_3_IRQ (3 + 32 + 16)

/* Helper macro */
#define BIT(n) (1U << (n))

/* Pointer to the counter low register for reading current time */
static volatile uint32_t *system_clock;

struct bcm2837_timer {
    volatile uint32_t *control;
    volatile uint32_t *compare;
    uint32_t match_mask;
    struct clock_event_device event_dev;
    struct irqaction irqaction;
};

static void bcm2837_timer_set_next_event(unsigned long event, struct clock_event_device *dev);
static irqreturn_t bcm2837_timer_interrupt_handler(unsigned int irq, void *dev_id);

static struct bcm2837_timer bcm_timer = {
    .control = (volatile uint32_t *)(BCM2837_TIMER_BASE + REG_CONTROL),
    .compare = (volatile uint32_t *)(BCM2837_TIMER_BASE + REG_COMPARE(DEFAULT_TIMER)),
    .match_mask = TIMER_MATCH_MASK,
    .event_dev = {
        .name = "bcm2837-system-timer",
        .event_handler = NULL,
        .set_next_event = bcm2837_timer_set_next_event,
    },
    .irqaction = {
        .handler = bcm2837_timer_interrupt_handler,
        .dev_id = NULL,     /* Set in init */
        .next = NULL,
        .flags = IRQF_TIMER,
    },
};

/*
 * bcm2837_timer_set_next_event - Program the next timer interrupt
 * @event: Time in nanoseconds for the next event (absolute or delta)
 * @now: Current time (unused in this implementation)
 *
 * Converts the event time to timer ticks and writes to the compare register.
 * The timer runs at 1MHz, so 1 tick = 1 microsecond.
 */
static void bcm2837_timer_set_next_event(unsigned long event, struct clock_event_device *dev)
{
    uint32_t current_counter;
    /* Read current counter value and add delta */
    current_counter = *system_clock;
    
    /* Write the compare value for when the interrupt should fire */
    *bcm_timer.compare = current_counter + event;
}

/*
 * bcm2837_timer_interrupt_handler - Handle timer interrupt
 * @irq: IRQ number
 * @dev_id: Device identifier (pointer to bcm2837_timer)
 */
static irqreturn_t bcm2837_timer_interrupt_handler(unsigned int irq, void *dev_id)
{
    struct bcm2837_timer *timer = dev_id;
    void (*event_handler)(struct clock_event_device *);
    
    /* Check if this interrupt is from our timer channel */
    if (*timer->control & timer->match_mask) {
        /* Clear the match flag by writing 1 to it */
        *timer->control = timer->match_mask;
        
        /* Call the event handler if registered */
        event_handler = timer->event_dev.event_handler;
        if (event_handler)
            event_handler(&timer->event_dev);
        
        return IRQ_HANDLED;
    }
    
    return IRQ_NONE;
}

int bcm2837_timer_init(void)
{
    int ret;
    
    /* Initialize the system clock pointer to counter low register */
    system_clock = (volatile uint32_t *)(BCM2837_TIMER_BASE + REG_COUNTER_LOW);
    
    /* Clear any pending match on our timer channel */
    *bcm_timer.control = bcm_timer.match_mask;
    
    /* Set up the IRQ action */
    bcm_timer.irqaction.dev_id = &bcm_timer;
    
    /* Register the IRQ handler */
    ret = request_irq(SYSTEM_TIMER_3_IRQ, 
                      bcm2837_timer_interrupt_handler,
                      IRQF_TIMER,
                      &bcm_timer);
    if (ret) {
        /* Failed to register IRQ */
        return ret;
    }
    
    /* Enable the IRQ */
    enable_irq(SYSTEM_TIMER_3_IRQ);
    
    /* Register the clock event device - this will also program the first tick */
    clockevents_config_and_register(&bcm_timer.event_dev);
    
    return 0;
}