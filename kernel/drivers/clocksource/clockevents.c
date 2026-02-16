#include <stdint.h>
#include <kernel/clockchip.h>
#include <kernel/timekeeping.h>

/*
 * Tick period in timer ticks (microseconds for 1MHz timer).
 * 10ms = 10,000 microseconds = 10,000 ticks
 */
#define TICK_PERIOD_US  10000UL  /* 10ms */

static struct clock_event_device *default_clockevent;

/*
 * Event handler for periodic ticks.
 * This is called by the hardware timer interrupt.
 */
void tick_periodic_clockevent(struct clock_event_device *dev)
{
    /* Update the jiffies counter */
    do_timer(1);

    /* Program the next tick event (delta from now) */
    if (dev->set_next_event) {
        dev->set_next_event(TICK_PERIOD_US, dev);
    }
}

/*
 * Setup the clock event device for periodic ticks.
 */
static void tick_setup_periodic(struct clock_event_device *dev)
{
    /* Set the periodic tick handler */
    dev->event_handler = tick_periodic_clockevent;

    /* Program the first tick */
    if (dev->set_next_event) {
        dev->set_next_event(TICK_PERIOD_US, dev);
    }
}

/*
 * Configure and register a clock event device.
 */
void clockevents_config_and_register(struct clock_event_device *dev)
{
    /* Set as the default clock event device */
    default_clockevent = dev;

    /* Setup for periodic tick mode */
    tick_setup_periodic(dev);
}