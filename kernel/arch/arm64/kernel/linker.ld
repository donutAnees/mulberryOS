/*
 * IMPORTANT:
 * ----------
 * This kernel currently runs with the MMU DISABLED.
 *
 * As a result:
 *   - All addresses are PHYSICAL addresses
 *   - There is no virtual memory
 *
 * Therefore:
 *   LINK-TIME ADDRESSES == RUNTIME ADDRESSES!! 
 *
 * The kernel MUST be linked at the exact physical address where the firmware
 * loads it. Any mismatch will result in invalid absolute references and
 * unexpected behavior like suppose you define a global variable, at compile
 * time the compiler will generate code to access it at a fixed physical and
 * later at runtime if the kernel loads at a different physical address and 
 * tries to access that variable, it will end up r/w to the wrong address.
 *
 * Once an MMU and virtual memory are introduced, this constraint can be
 * relaxed by switching to a virtual kernel address space. Until then, this
 * fixed layout is REQUIRED. I know this sucks but don't worry we will fix this.
 *
 * The Raspberry Pi firmware always loads AArch64 kernels at physical
 * address 0x80000 and jumps directly to it with the MMU disabled.
 *
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

    /*
     * Exception vector table
     */
    .text.vectors : {
        *(.text.vectors)
    }

    /*
     * Main kernel code
     * All compiled C functions go here
     */
    .text : {
        *(.text)
        *(.text.*)
    }

    /*
     * Read-only data
     * String literals, const variables, etc.
     */
    .rodata : ALIGN(8) {
        *(.rodata)
        *(.rodata.*)
    }

    /*
     * Initialized data
     * Global and static variables with initial values
     */
    .data : ALIGN(8) {
        *(.data)
        *(.data.*)
    }

    /*
     * Uninitialized data (BSS)
     * Global and static variables without initializers
     * Must be zeroed by boot.S before C code runs
     */
    .bss : ALIGN(16) {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /*
     * End of kernel marker
     * Used by boot.S to calculate kernel size
     * and position the initial stack
     */
    _end = .;
    
    .bootstrap_stack (NOLOAD) : {
        *(.bootstrap_stack)
    }
}

/*
 * Calculate BSS size for reference
 */
__bss_size = (__bss_end - __bss_start);