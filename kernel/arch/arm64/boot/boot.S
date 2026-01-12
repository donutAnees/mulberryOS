// ------------------------------------------------------
// Bare-metal bootloader for Raspberry Pi Zero 2 (AArch64)
// Only Core 0 runs; other cores are held in WFE (Wait For Event) state
// ------------------------------------------------------
// Boot sequence:
//   1. Firmware loads kernel to 0x80000, jumps to _start at EL2
//   2. Drop from EL2 to EL1 (if needed)
//   3. Clear BSS section
//   4. Build page tables (identity + higher-half)
//   5. Configure and enable MMU
//   6. Set up kernel stack
//   7. Jump to C kernel entry point
//
// Firmware state on entry:
//   - Exception Level: EL2 (hypervisor mode)
//   - MMU: Disabled
//   - Caches: Disabled
//   - x0: Device Tree Blob (DTB) pointer
// ------------------------------------------------------

#include <asm/mmu.h>

// --------------------------------------------------
// Bootstrap Stack (16KB)
// --------------------------------------------------
.section ".bootstrap_stack", "aw", %nobits
.align 16
stack_bottom:
    .space 0x4000               // 16KB bootstrap stack
stack_top:

.section ".bss"
/// --------------------------------------------------
// Level 0 translation table (shared by TTBR0 & TTBR1)
// --------------------------------------------------
.align 12
page_table_l0:
    .space 4096 // We do not need these many entries for our Pi
                // TODO: Optimize to have the minimal entry of 1 table or whatever

// --------------------------------------------------
// Level 1 table 
// --------------------------------------------------
.align 12
page_table_l1:
    .space 4096 // TODO: Optimize to have the minimal entry of 1 table or whatever

// --------------------------------------------------
// Level 2 table (2MB block mappings)
// --------------------------------------------------
.align 12
page_table_l2:
    .space 4096 // TODO: Optimize to have the minimal entry of 1 table or whatever

.section ".text.boot"
.globl _start

_start:
    // ==============================================================================
    // Step 1: Exception Level Transition (EL2 → EL1)
    // ==============================================================================
    // Check if we're running at EL2 (hypervisor mode)
    // If already at EL1, skip this section

    mrs     x5, CurrentEL
    lsr     x5, x5, #2        // Extract EL number (bits [3:2])
    cmp     x5, #2
    b.ne    el1_entry         // If already EL1, skip EL2 setup

    // ---- Configure EL1 execution state ----

    // HCR_EL2: Hypervisor Configuration Register
    // Bit 31 (RW) = 1: EL1 uses AArch64 (not AArch32)
    mov     x5, #(1 << 31)
    msr     hcr_el2, x5

    // SCTLR_EL2: Disable MMU and caches at EL2
    mov     x5, #0
    msr     sctlr_el2, x5
    isb

    // ELR_EL2: Set exception return address to el1_entry
    adr     x5, el1_entry
    msr     elr_el2, x5

    // SPSR_EL2: Set processor state for EL1 entry
    // Bits [3:0] = 0b0101 (EL1h - use SP_EL1, not SP_EL0)
    // Bits [9:6] = 0b1111 (mask DAIF - all interrupts masked)
    mov     x5, #0x3C5        // 0b0011_1100_0101
    msr     spsr_el2, x5

    eret                      // Exception return: drops to EL1

el1_entry:
    // ==============================================================================
    // Step 2: Clear BSS Section
    // ==============================================================================
    // Zero all uninitialized global/static variables
    // Must complete before any C code accesses these variables
    // We substract KERNEL_VA_BASE to get physical addresses
    // The linker places them at higher half virtual addresses
    // ----------------------------------------------------------------------
    ldr     x5, =__bss_start - KERNEL_VA_BASE  // x5 = start of BSS
    ldr     x6, =__bss_end - KERNEL_VA_BASE    // x6 = end of BSS

1:  
    cmp     x5, x6            // Check if done
    b.hs    2f                // Branch if x5 >= x6
    str     xzr, [x5], #8     // Zero 8 bytes, post-increment
    b       1b                // Continue loop

2:
    // ==============================================================================
    // Step 3: Configure MMU Registers and Set Up Page Tables
    // ==============================================================================
    // Configure MAIR, TCR, and TTBR registers before enabling MMU
    // ----------------------------------------------------------------------

    ldr    x5, =TCR_EL1_VALUE
    msr    tcr_el1, x5               // Set TCR_EL1

    ldr    x5, =MAIR_EL1_VALUE
    msr    mair_el1, x5               // Set memory attributes 

    // Set TTBR0_EL1 to point to level 0 page table
    ldr    x5, =page_table_l0 - KERNEL_VA_BASE
    ldr    x6, =page_table_l1 - KERNEL_VA_BASE
    ldr    x7, =page_table_l2 - KERNEL_VA_BASE

    // -------------------------------------------------------------
    // Build page tables
    // -------------------------------------------------------------
    // L0[0] -> L1 table
    orr    x8, x6, #PTE_TYPE_TABLE          // table descriptor (bits[1:0] = 11)
    str    x8, [x5]

    // L1[0] -> L2 table
    orr    x8, x7, #PTE_TYPE_TABLE          // table descriptor
    str    x8, [x6]

    // -------------------------------------------------------------
    // Fill L2 table with 2MB blocks
    // Raspberry Pi 3 and Zero 2W address map:
    //   RAM                : 0x00000000 – 0x3DFFFFFF
    //   GPU Peripherals    : 0x3E000000 – 0x3FFFFFFF
    //   Local Peripherals  : 0x40000000 – 0x4001FFFF (ARM timer, IRQs, mailboxes)
    // -------------------------------------------------------------

    mov    x9, #0                // L2 index = 0

1:
    // Physical address = index * 2MB
    lsl    x10, x9, #21

    // Default: Normal memory (RAM)
    ldr    x11, =PTE_BLOCK_NORMAL   // Normal WB WA, AF=1, SH=Inner

    // Check if >= 0x3E000000 (GPU peripherals)
    ldr    x13, =GPU_PERIPH_BASE_PA
    cmp    x10, x13
    b.lo   2f
    ldr    x11, =PTE_BLOCK_DEVICE   // Device-nGnRnE, PXN+UXN
    
    // Check if >= 0x40000000 (Local ARM peripherals)
    ldr    x13, =LOCAL_PERIPH_BASE_PA
    cmp    x10, x13
    b.lo   2f
    ldr    x11, =PTE_BLOCK_DEVICE   // Device-nGnRnE, PXN+UXN

2:
    orr    x12, x10, x11
    str    x12, [x7, x9, lsl #3]

    add    x9, x9, #1
    cmp    x9, #512               // map first 1GB (512 × 2MB)
    b.ne   1b

    msr    ttbr0_el1, x5
    msr    ttbr1_el1, x5               // using same table for TTBR1_EL1

    // ---- SCTLR_EL1: System Control Register ----
    // Implemented in CPUECTLR register
    mrs     x5, S3_1_C15_C2_1
    orr     x5, x5, #(0x1 << 6)     // SMP bit for hardware coherency
    msr     S3_1_C15_C2_1, x5
    
    mrs     x5, sctlr_el1
    orr     x5, x5, #(1 << 0)       // M = 1 (Enable MMU)
    orr     x5, x5, #(1 << 2)       // C = 1 (Enable D-cache)
    orr     x5, x5, #(1 << 12)      // I = 1 (Enable I-cache)
    msr     sctlr_el1, x5
    dsb     sy                      // Ensure MMU enable completes
    isb                             // Synchronize after enabling MMU
    
    // ==============================================================================
    // Step 4: Jump to Higher Half
    // ==============================================================================
    // MMU is now enabled with dual mapping (identity + higher-half)
    // We're still executing at low physical addresses
    // Jump to higher-half virtual address

    ldr     x5, =higher_half_entry         // Load higher-half virtual address
    br      x5                             // Branch to higher-half

.section ".text.higher_half"
higher_half_entry:
    // We are now running in higher-half kernel space
    // Virtual addresses >= 0xFFFF000000000000 are active
    
    // ==============================================================================
    // Step 5: Set Up Kernel Stack
    // ==============================================================================
    // Stack grows downward from stack_top
    // Must be set up before calling C code

    ldr     x5, =stack_top    // Load address of top of stack
    mov     sp, x5            // Set stack pointer

    // ==============================================================================
    // Step 6: Jump to Kernel Entry Point
    // ==============================================================================
    // Transfer control to C kernel
    // kernel_main() never returns

    bl      kernel_main

// ==============================================================================
// Failsafe Halt Loop
// ==============================================================================
// If kernel_main returns (should never happen), halt the CPU

halt:
    wfe                       // Wait For Event (low-power idle)
    b       halt              // Infinite loop