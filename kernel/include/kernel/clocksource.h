#ifndef KERNEL_CLOCKSOURCE_H
#define KERNEL_CLOCKSOURCE_H

#include <stdint.h>

/**
 * struct clocksource - hardware time counter abstraction
 *
 * A clocksource represents a free-running hardware counter that the
 * we can read to determine the current time. It does NOT track
 * time itself, it only provides raw cycles and the parameters needed
 * to convert them to nanoseconds.
 */
struct clocksource {
	/**
	 * @read:
	 * Callback used to read the current value of the hardware counter.
	 *
	 * The clocksource pointer is passed so the function can access
	 * clocksource-specific data (registers, MMIO base, masks, etc.).
	 *
	 * Must return a monotonically increasing value (modulo @mask).
	 */
	uint64_t (*read)(struct clocksource *cs);

	/**
	 * @mask:
	 * Bitmask applied to the raw counter value.
	 *
	 * Used to handle counters that are smaller than 64 bits
	 * (for example, 24-bit or 32-bit hardware timers) and allow
	 * correct wrap-around handling.
     * 
     * Example: A 32-bit timer would have a mask of 0xFFFFFFFF.
     * This ensures that only the lower 32 bits of the counter
     * value are considered, and any overflow beyond that is
     * correctly handled by wrapping around to zero.
     * 
	 */
	uint64_t mask;

	/**
	 * @mult:
	 * Multiplier used to convert counter cycles into nanoseconds.
	 *
	 * Part of a fixed-point conversion formula to avoid expensive
	 * division operations.
     * 
     * The conversion from cycles to nanoseconds is done using the formula:
     * nanoseconds = (cycles * mult) >> shift
     * Here, 'cycles' is the raw counter value read from the hardware,
     * 'mult' is the multiplier, and 'shift' is the right shift value
     * 
     * mult is calculated based on the clock frequency of the hardware timer:
     * mult = (1,000,000,000 << shift) / frequency
     * where 1,000,000,000 is the number of nanoseconds in one second.
     * frequency is the timer's frequency in Hz.
	 */
	uint32_t mult;

	/**
	 * @shift:
	 * Right shift value paired with @mult.
	 * The shift value is used to scale down the result of the multiplication
     * to achieve the correct time unit (nanoseconds).
     * 
	 */
	uint32_t shift;

	/**
	 * @name:
	 *  Human-readable name of the clocksource.
	 *
	 */
	const char *name;
};

# endif /* KERNEL_CLOCKSOURCE_H */