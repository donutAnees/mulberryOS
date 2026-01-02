/*
 * Higher-half kernel linker script (AArch64)
 *
 * Kernel is:
 *   - LOADED at PA 0x80000
 *   - LINKED at VA 0xFFFF000000080000
 *
 * MEMORY LAYOUT (PHYSICAL ADDRESSES)
 *
 *  High Address
 *  ┌─────────────────────────────────┐
 *  │                                 │
 *  │      Available RAM              │
 *  │                                 │
 *  ├─────────────────────────────────┤
 *  │  Stack (16KB, grows DOWN)       │
 *  │          ↓                      │  SP initialized in boot.S
 *  │          ↓                      │  using _end symbol
 *  │          ↓                      │
 *  ├─────────────────────────────────┤  ← _end (end of kernel image)
 *  │                                 │
 *  │  .bss (uninitialized data)      │
 *  │  Zeroed manually in boot.S      │
 *  │                                 │
 *  ├─────────────────────────────────┤
 *  │  .data (initialized data)       │
 *  │                                 │
 *  ├─────────────────────────────────┤
 *  │  .rodata (read-only data)       │
 *  │                                 │
 *  ├─────────────────────────────────┤
 *  │  .text (kernel code)            │
 *  │                                 │
 *  ├─────────────────────────────────┤
 *  │  .text.boot (early boot code)   │
 *  │                                 │
 *  ├─────────────────────────────────┤  ← _start = 0x80000
 *  │                                 │     Kernel entry point
 *  │  Raspberry Pi Firmware          │
 *  │                                 │
 *  └─────────────────────────────────┘
 *  Low Address
 *
 */

ENTRY(_start)

KERNEL_VA_BASE = 0xFFFF000000000000;

SECTIONS
{
    /*
     * Link at 0x80000 - the load address used by Raspberry Pi bootloader.
     */
    . = 0x80000;

    /*
     * .text.boot section must come first
     * Contains boot.S code with entry point
     */
    .text.boot : {
        *(.text.boot)
    }

    . += KERNEL_VA_BASE;
    /*
     * Exception vector table
     */
    .text.vectors : AT(ADDR(.text.vectors) - KERNEL_VA_BASE) {
        *(.text.vectors)
    }

    /*
     * Main kernel code
     * All compiled C functions go here
     */
    .text : AT(ADDR(.text) - KERNEL_VA_BASE) {
        *(.text)
        *(.text.*)
    }

    /*
     * Read-only data
     * String literals, const variables, etc.
     */
    .rodata ALIGN(8) : AT(ADDR(.rodata) - KERNEL_VA_BASE) {
        *(.rodata)
        *(.rodata.*)
    }

    /*
     * Initialized data
     * Global and static variables with initial values
     */
    .data ALIGN(8) : AT(ADDR(.data) - KERNEL_VA_BASE) {
        *(.data)
        *(.data.*)
    }

    /*
     * Uninitialized data (BSS)
     * Global and static variables without initializers
     * Must be zeroed by boot.S before C code runs
     */
    .bss ALIGN(16) : AT(ADDR(.bss) - KERNEL_VA_BASE) {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        *(.bootstrap_stack)
        __bss_end = .;
    }
}