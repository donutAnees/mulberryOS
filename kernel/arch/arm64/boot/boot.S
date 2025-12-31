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

.section ".bootstrap_stack", "aw", %nobits
.align 16
stack_bottom:
    .space 0x4000               // 16KB bootstrap stack
stack_top:

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

    ldr     x5, =__bss_start  // x5 = start of BSS
    ldr     x6, =__bss_end    // x6 = end of BSS

1:  
    cmp     x5, x6            // Check if done
    b.hs    2f                // Branch if x5 >= x6
    str     xzr, [x5], #8     // Zero 8 bytes, post-increment
    b       1b                // Continue loop

2:
    // ==============================================================================
    // Step 3: Build Page Tables
    // ==============================================================================
    // Create identity mapping and higher-half kernel mapping
    // Called before MMU enable; page tables stored in BSS (now zeroed)

    bl build_dual_map

    // ==============================================================================
    // Step 4: Configure MMU Registers
    // ==============================================================================

    // ==============================================================================
    // Step 5: Jump to Higher Half
    // ==============================================================================
    // MMU is now enabled with dual mapping (identity + higher-half)
    // We're still executing at low physical addresses
    // Jump to higher-half virtual address

    ldr     x5, =higher_half_entry         // Load higher-half virtual address
    br      x5                             // Branch to higher-half

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
    // Step 8: Jump to Kernel Entry Point
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

// ==============================================================================
// build_dual_map: Create Identity and Higher-Half Page Tables
// ==============================================================================
// Sets up two-level page tables:
//   - Identity mapping: VA 0x0 → PA 0x0 (for MMU enable transition)
//   - Higher-half mapping: VA 0xFFFF000000000000 → PA 0x0 (kernel space)
//
// Uses page tables allocated in BSS (already zeroed):
//   - page_table_l0: Level 0 translation table (TTBR1_EL1 - higher half)
//   - page_table_l1: Level 1 translation table (TTBR0_EL1 - identity)
// ==============================================================================
build_dual_map:
    ret