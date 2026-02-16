#ifndef CLOCKCHIP_H
#define CLOCKCHIP_H

#include <stdint.h>

struct clock_event_device;
void clockevents_config_and_register(struct clock_event_device *dev);
void tick_periodic_clockevent(struct clock_event_device *dev);

/* 
* clocks_event_device represents a hardware timer 
* that can be programmed to generate interrupts at specific times. 
* It provides a callback function set_next_event that the kernel 
* can call to schedule the next timer event.
*/
struct clock_event_device {
    /**
     * @event_handler:
     * Callback function that is called when the timer interrupt occurs.
     */
    void (*event_handler)(struct clock_event_device *);
    /**
     * @set_next_event:
     * Callback used to program the hardware timer to generate an interrupt
     * at a specific time in the future.
     * @event: Delta in timer ticks (microseconds for 1MHz timer)
     * @evt_dev: The clock event device
     */
    void (*set_next_event)(unsigned long event, struct clock_event_device *evt_dev);
    /* 
     * name: human readable name of the device 
     */
    const char *name;
};

#endif /* CLOCKCHIP_H */