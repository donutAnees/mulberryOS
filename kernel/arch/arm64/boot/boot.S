// ------------------------------------------------------
// Bare-metal bootloader for Raspberry Pi Zero 2 (AArch64)
// Only Core 0 runs; other cores are held in WFE (Wait For Event) state
// Firmware passes:
//   x0 = Device Tree Blob (DTB) pointer (32-bit address)
// ------------------------------------------------------

.section ".text.boot"
.globl _start

_start:

    // --------------------------------------------------
    // Step 1: Establish a temporary stack
    // Stack will be placed just after the kernel + 16KB space
    // Note: ARM stack grows downward
    // --------------------------------------------------

    ldr     x5, =_end         // x5 = address of end of kernel
    add     x5, x5, #0x4000   // x5 = top of stack = end of kernel + 16KB
    mov     sp, x5            // Set SP (stack pointer) to top of stack

    // At this point:
    // SP points to temporary stack (16KB) above the kernel.
    // This stack is used during initialization before main kernel runs.

    // --------------------------------------------------
    // Step 2: Clear .bss section
    // .bss contains all uninitialized global/static variables
    // We must zero it before using these variables in C code
    // --------------------------------------------------

    ldr     x5, =__bss_start  // x5 = address of __bss_start
    ldr     x6, =__bss_end    // x6 = address of __bss_end

1:  // Loop to clear .bss
    cmp     x5, x6            // Compare current position with end
    b.hs    2f                // If >= end, we're done
    str     xzr, [x5], #8     // Store zero to memory and increment pointer by 8 bytes
    b       1b                // Loop until all .bss cleared

    // After this loop:
    // All uninitialized globals/statics in kernel are zeroed.

    // --------------------------------------------------
    // Step 3: Jump to main C kernel
    // --------------------------------------------------

2:
    bl      kernel_main        // Branch with link to kernel_main
                               // Return address will not be used; kernel_main never returns

// ------------------------------------------------------
// Failsafe halt loop
// In case kernel_main returns, just halt the CPU
// ------------------------------------------------------
halt:
    wfe                       // Wait For Event (low-power sleep)
    b       halt               // Infinite loop