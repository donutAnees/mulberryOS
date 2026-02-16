#ifndef _JIFFIES_H
#define _JIFFIES_H

#include <stdint.h>

/*
 * This counter is incremented on every timer interrupt and represents
 * the number of ticks since system boot.
 *
 * We define only the 64-bit version for simplicity. In contrast, Linux
 * provides both a 32-bit and a 64-bit version: the 32-bit jiffies variable points
 * to the lower 32 bits of jiffies_64, while the 64-bit jiffies variable 
 * points to jiffies_64.
 */
extern uint64_t jiffies_64;

/*
 * jiffies is an unsigned counter that wraps:
 *
 *   ... 254, 255, 0, 1, 2 ...
 *
 * Because of wraparound, simple (a > b) comparisons fail across
 * the boundary. Instead we subtract and interpret the result as
 * signed.
 *
 * When the counter wraps, values that are numerically small may
 * actually represent later points in time. Subtracting first in
 * unsigned arithmetic preserves the correct modular distance.
 * Interpreting that distance as signed places values from the
 * upper half of the range into the negative space, which allows
 * us to detect wraparound correctly. 
 *
 * This works as long as the time difference is less than half
 * the counter range.
 */
#define time_after(a,b) ((int64_t)((b) - (a)) < 0)

#define time_before(a,b) time_after(b,a)

#define time_after_jiffies(a) time_after(jiffies,a)

#define time_before_jiffies(a) time_before(jiffies,a)

#endif