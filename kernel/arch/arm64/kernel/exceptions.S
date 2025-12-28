// ==============================================================================
// Exception Vector Table for ARMv8-A (AArch64)
// ==============================================================================
//
// This file implements the exception vector table according to the ARMv8-A
// architecture specification. The vector table handles all exceptions for EL1.
//
// Key Requirements:
// - Each vector entry is exactly 128 bytes (0x80)
// - 16 total entries organized in 4 groups of 4
//
// Exception Types:
// - Synchronous: Data/instruction aborts, undefined instructions, SVC, etc.
// - IRQ: Standard interrupts
// - FIQ: Fast interrupts (higher priority than IRQ)
// - SError: System errors (asynchronous external aborts)
//
// ==============================================================================

// ==============================================================================
// Exception Context Macros
// ==============================================================================

/**
 * kernel_entry - Save processor state on exception entry
 * 
 * Saves all general-purpose registers (x0-x30), exception link register (ELR),
 * and saved program status register (SPSR) to the stack.
 * 
 * Stack frame layout (each register pair = 16 bytes):
 *   SP + 0x00: x0, x1
 *   SP + 0x10: x2, x3
 *   ...
 *   SP + 0xE0: x28, x29
 *   SP + 0xF0: x30 (LR), ELR_EL1
 *   SP + 0x100: SPSR_EL1 (8 bytes)
 * 
 * Total stack usage: 264 bytes (0x108)
 */
.macro kernel_entry
    // Allocate stack space for all registers
    sub     sp, sp, #272            // 17 pairs * 16 + 16 for alignment
    
    // Save general-purpose registers (pairs for efficiency)
    stp     x0, x1,   [sp, #16 * 0]
    stp     x2, x3,   [sp, #16 * 1]
    stp     x4, x5,   [sp, #16 * 2]
    stp     x6, x7,   [sp, #16 * 3]
    stp     x8, x9,   [sp, #16 * 4]
    stp     x10, x11, [sp, #16 * 5]
    stp     x12, x13, [sp, #16 * 6]
    stp     x14, x15, [sp, #16 * 7]
    stp     x16, x17, [sp, #16 * 8]
    stp     x18, x19, [sp, #16 * 9]
    stp     x20, x21, [sp, #16 * 10]
    stp     x22, x23, [sp, #16 * 11]
    stp     x24, x25, [sp, #16 * 12]
    stp     x26, x27, [sp, #16 * 13]
    stp     x28, x29, [sp, #16 * 14]
    
    // Save x30 (link register) and ELR_EL1 (exception return address)
    mrs     x21, elr_el1
    stp     x30, x21, [sp, #16 * 15]
    
    // Save SPSR_EL1 (saved processor state)
    mrs     x22, spsr_el1
    str     x22, [sp, #16 * 16]
.endm

/**
 * kernel_exit - Restore processor state and return from exception
 * 
 * Restores all saved registers from the stack and executes eret to return
 * from the exception handler.
 */
.macro kernel_exit
    // Restore SPSR_EL1
    ldr     x22, [sp, #16 * 16]
    msr     spsr_el1, x22
    
    // Restore x30 and ELR_EL1
    ldp     x30, x21, [sp, #16 * 15]
    msr     elr_el1, x21
    
    // Restore general-purpose registers
    ldp     x28, x29, [sp, #16 * 14]
    ldp     x26, x27, [sp, #16 * 13]
    ldp     x24, x25, [sp, #16 * 12]
    ldp     x22, x23, [sp, #16 * 11]
    ldp     x20, x21, [sp, #16 * 10]
    ldp     x18, x19, [sp, #16 * 9]
    ldp     x16, x17, [sp, #16 * 8]
    ldp     x14, x15, [sp, #16 * 7]
    ldp     x12, x13, [sp, #16 * 6]
    ldp     x10, x11, [sp, #16 * 5]
    ldp     x8, x9,   [sp, #16 * 4]
    ldp     x6, x7,   [sp, #16 * 3]
    ldp     x4, x5,   [sp, #16 * 2]
    ldp     x2, x3,   [sp, #16 * 1]
    ldp     x0, x1,   [sp, #16 * 0]
    
    // Deallocate stack space
    add     sp, sp, #272
    
    // Return from exception
    eret
.endm

/* =========================================================
 * Vector slot macro (128 bytes exactly)
 * ========================================================= */

.macro VECTOR_SLOT target
    .align 7
.Lvector_start\@:
    b \target
.org .Lvector_start\@ + 128
.endm

// ==============================================================================
// Exception Vector Table
// ==============================================================================

.section ".text.vectors", "ax"
.align 11                   // Align to 2048 bytes (2KB boundary specified by ARMv8-A)

.globl exception_vector_table
exception_vector_table:

/* Current EL, SP_EL0 */
VECTOR_SLOT el1_sp0_sync
VECTOR_SLOT el1_sp0_irq
VECTOR_SLOT el1_sp0_fiq
VECTOR_SLOT el1_sp0_serror

/* Current EL, SP_EL1 */
VECTOR_SLOT el1_spx_sync
VECTOR_SLOT el1_spx_irq
VECTOR_SLOT el1_spx_fiq
VECTOR_SLOT el1_spx_serror

/* Lower EL, AArch64 */
VECTOR_SLOT lower_el64_sync
VECTOR_SLOT lower_el64_irq
VECTOR_SLOT lower_el64_fiq
VECTOR_SLOT lower_el64_serror

/* Lower EL, AArch32 */
VECTOR_SLOT lower_el32_sync
VECTOR_SLOT lower_el32_irq
VECTOR_SLOT lower_el32_fiq
VECTOR_SLOT lower_el32_serror

/* =========================================================
 * Actual handlers (outside vector table)
 * ========================================================= */

el1_sp0_sync:
    KERNEL_ENTRY
    mov     x0, #0
    bl      exception_handler_c
    KERNEL_EXIT

el1_sp0_irq:
    KERNEL_ENTRY
    mov     x0, #1
    bl      exception_handler_c
    KERNEL_EXIT

el1_sp0_fiq:
    KERNEL_ENTRY
    mov     x0, #2
    bl      exception_handler_c
    KERNEL_EXIT

el1_sp0_serror:
    KERNEL_ENTRY
    mov     x0, #3
    bl      exception_handler_c
    KERNEL_EXIT

el1_spx_sync:
    KERNEL_ENTRY
    mov     x0, #4
    bl      exception_handler_c
    KERNEL_EXIT

el1_spx_irq:
    KERNEL_ENTRY
    mov     x0, #5
    bl      exception_handler_c
    KERNEL_EXIT

el1_spx_fiq:
    KERNEL_ENTRY
    mov     x0, #6
    bl      exception_handler_c
    KERNEL_EXIT

el1_spx_serror:
    KERNEL_ENTRY
    mov     x0, #7
    bl      exception_handler_c
    KERNEL_EXIT

lower_el64_sync:
    KERNEL_ENTRY
    mov     x0, #8
    bl      exception_handler_c
    KERNEL_EXIT

lower_el64_irq:
    KERNEL_ENTRY
    mov     x0, #9
    bl      exception_handler_c
    KERNEL_EXIT

lower_el64_fiq:
    KERNEL_ENTRY
    mov     x0, #10
    bl      exception_handler_c
    KERNEL_EXIT

lower_el64_serror:
    KERNEL_ENTRY
    mov     x0, #11
    bl      exception_handler_c
    KERNEL_EXIT

lower_el32_sync:
    KERNEL_ENTRY
    mov     x0, #12
    bl      exception_handler_c
    KERNEL_EXIT

lower_el32_irq:
    KERNEL_ENTRY
    mov     x0, #13
    bl      exception_handler_c
    KERNEL_EXIT

lower_el32_fiq:
    KERNEL_ENTRY
    mov     x0, #14
    bl      exception_handler_c
    KERNEL_EXIT

lower_el32_serror:
    KERNEL_ENTRY
    mov     x0, #15
    bl      exception_handler_c
    KERNEL_EXIT

// ==============================================================================
// Install Exception Vector Table
// ==============================================================================

.globl install_exception_vectors
install_exception_vectors:
    adrp    x0, exception_vector_table      // Get page address
    add     x0, x0, :lo12:exception_vector_table
    msr     vbar_el1, x0                    // Set vector base address register
    isb                                      // Instruction synchronization barrier
    ret
