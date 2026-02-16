# Time Management

Following a similar time management implementation available in the Linux kernel.

The main components of the time management subsystem are:
- **clocksource**: Hardware abstraction for an n-bit counter which reads the current time.
- **clockevent**: Program the hardware to generate an interrupt after a specified time has elapsed.
- **jiffies**: Global tick counter incremented on every timer interrupt.
- **sched_clock**: Weak function returning current time in nanoseconds (not implemented).

## Jiffies

Jiffies is a global 64-bit counter incremented on every timer interrupt. It represents the number of ticks since system boot.

Updated by `do_timer()` which is called from the clockevent handler on each tick.

## Clocksource

A clocksource represents a free-running hardware counter for reading the current time. It does NOT track time itself — only provides raw cycles and parameters to convert them to nanoseconds.

```c
// kernel/include/kernel/clocksource.h
struct clocksource {
    uint64_t (*read)(struct clocksource *cs);
    uint64_t mask;      // Bitmask for counter width (e.g., 0xFFFFFFFF for 32-bit)
    uint32_t mult;      // Multiplier for cycles → nanoseconds
    uint32_t shift;     // Shift for fixed-point conversion
    const char *name;
};
```

**Conversion formula**: `nanoseconds = (cycles * mult) >> shift`

**NOTE**: Clocksource is defined but not actively used yet. The timer driver reads the counter directly.

## Clockevent

The clockevent provides a way to program the hardware to generate an interrupt after a specified time.

```c
// kernel/include/kernel/clockchip.h
struct clock_event_device {
    void (*event_handler)(struct clock_event_device *);
    void (*set_next_event)(unsigned long event, struct clock_event_device *evt_dev);
    const char *name;
};
```

**Current implementation** uses periodic mode with a 10ms tick period:

## BCM2837 System Timer Driver

The BCM2837 SoC has a 1MHz free-running counter with 4 compare channels. We use channel 3 for the kernel tick.

## Timer Tick Flow

1. `bcm2837_timer_init()` registers IRQ handler for virq 51
2. `clockevents_config_and_register()` sets `event_handler = tick_periodic_clockevent`
3. First `set_next_event(10000)` programs C3 compare register
4. Timer fires when counter matches C3
5. IRQ handler calls `event_handler()`
6. `tick_periodic_clockevent()` updates jiffies and programs next tick
7. Repeat forever

## Reference
- [Linux clocksource documentation](https://docs.kernel.org/timers/timekeeping.html)