#include <kernel/jiffies.h>
#include <kernel/timekeeping.h>

/* Global tick counter - incremented on every timer interrupt */
uint64_t jiffies_64 = 0;

void do_timer(unsigned long ticks) {
    jiffies_64 += ticks;
}