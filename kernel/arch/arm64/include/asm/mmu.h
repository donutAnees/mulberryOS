#ifndef _ASM_MMU_H
#define _ASM_MMU_H

#ifndef __ASSEMBLY__
#define _UL(x)   (x##UL)
#else
#define _UL(x)   (x)
#endif

#define KERNEL_VA_BASE    _UL(0xffff000000000000)
#define GPU_PERIPH_BASE_PA    _UL(0x3E000000)  /* GPU peripheral access */
#define LOCAL_PERIPH_BASE_PA  _UL(0x40000000)  /* Local peripherals (ARM timer, IRQs, mailboxes) */

/*
 * Page table levels for 4KB granule, 48-bit VA
 * 
 * Level 0 (L0): bits [47:39] - 512GB per entry (covers entire address space)
 * Level 1 (L1): bits [38:30] - 1GB per entry
 * Level 2 (L2): bits [29:21] - 2MB per entry (block mappings)
 * Level 3 (L3): bits [20:12] - 4KB per entry (page mappings)
 */
#define L0_SHIFT            39
#define L1_SHIFT            30
#define L2_SHIFT            21
#define L3_SHIFT            12

#define L0_SIZE             (_UL(1) << L0_SHIFT)    /* 512GB */
#define L1_SIZE             (_UL(1) << L1_SHIFT)    /* 1GB */
#define L2_SIZE             (_UL(1) << L2_SHIFT)    /* 2MB */
#define L3_SIZE             (_UL(1) << L3_SHIFT)    /* 4KB */

/* 
 * TCR_EL1 configuration
 */

/* TCR_EL1 field shifts */
#define TCR_PS_SHIFT      32
#define TCR_TG1_SHIFT     30
#define TCR_SH1_SHIFT     28
#define TCR_ORGN1_SHIFT   26
#define TCR_IRGN1_SHIFT   24
#define TCR_EPD1_SHIFT    23
#define TCR_A_SHIFT       22
#define TCR_T1SZ_SHIFT    16

#define TCR_TG0_SHIFT     14
#define TCR_SH0_SHIFT     12
#define TCR_ORGN0_SHIFT   10
#define TCR_IRGN0_SHIFT    8
#define TCR_EPD0_SHIFT     7
#define TCR_T0SZ_SHIFT     0

/* PA RANGE ID_AA64MMFR0_EL1 Shouldnt be defined here but fine for now */
#define ID_AA64MMFR0_PARANGE_32BIT   0b0000  /* 4 GB */
#define ID_AA64MMFR0_PARANGE_36BIT   0b0001  /* 64 GB */
#define ID_AA64MMFR0_PARANGE_40BIT   0b0010  /* 1 TB */
#define ID_AA64MMFR0_PARANGE_42BIT   0b0011  /* 4 TB */
#define ID_AA64MMFR0_PARANGE_44BIT   0b0100  /* 16 TB */
#define ID_AA64MMFR0_PARANGE_48BIT   0b0101  /* 256 TB */
/* When FEAT_LPA is implemented */
#define ID_AA64MMFR0_PARANGE_52BIT   0b0110  /* 4 PB */
/* When FEAT_D128 is implemented */
#define ID_AA64MMFR0_PARANGE_56BIT   0b0111  /* 64 PB */

/* TG0 */
#define TCR_TG0_4K     0b00

/* TG1 */
#define TCR_TG1_4K    0b10

/* Shareability both SH0 and SH1 have similar values */
#define TCR_SH_NON_SHAREABLE  0b00
#define TCR_SH_OUTER          0b10
#define TCR_SH_INNER          0b11

/* Cacheability both IRGN0/1 and ORGN0/1 have similar values */
#define TCR_RGN_NON_CACHEABLE  0b00
#define TCR_RGN_WRITE_BACK     0b01
#define TCR_RGN_WRITE_THROUGH  0b10
#define TCR_RGN_WRITE_ALLOCATE 0b11

/* Table Walk Enable */
#define TCR_EPD_ENABLE   0
#define TCR_EPD_DISABLE  1

/* ASID Selection */
#define TCR_ASID_TTBR0   0
#define TCR_ASID_TTBR1   1

/* Physical Address Size - PS */
#define TCR_T0SZ_48BIT   16
#define TCR_T1SZ_48BIT   16

/* For the following configuration this should give out the value 0x00000005b5103510 */
#define TCR_EL1_VALUE ( \
    /* Physical address size, will try to get this from the register later */ \
    ((ID_AA64MMFR0_PARANGE_48BIT & 0x7) << TCR_PS_SHIFT) | \
                                                   \
    /* TTBR1 configuration */                     \
    (TCR_TG1_4K         << TCR_TG1_SHIFT) | /* 4KB */ \
    (TCR_SH_INNER       << TCR_SH1_SHIFT) |        \
    (TCR_RGN_WRITE_BACK << TCR_ORGN1_SHIFT) |      \
    (TCR_RGN_WRITE_BACK << TCR_IRGN1_SHIFT) |      \
    (TCR_EPD_ENABLE     << TCR_EPD1_SHIFT) |       \
    (TCR_ASID_TTBR0     << TCR_A_SHIFT) |          \
    (TCR_T1SZ_48BIT     << TCR_T1SZ_SHIFT) |       \
                                                   \
    /* TTBR0 configuration */                     \
    (TCR_TG0_4K         << TCR_TG0_SHIFT) |       \
    (TCR_SH_INNER       << TCR_SH0_SHIFT) |       \
    (TCR_RGN_WRITE_BACK << TCR_ORGN0_SHIFT) |     \
    (TCR_RGN_WRITE_BACK << TCR_IRGN0_SHIFT) |     \
    (TCR_EPD_ENABLE     << TCR_EPD0_SHIFT) |      \
    (TCR_T0SZ_48BIT     << TCR_T0SZ_SHIFT)         \
)

/*
 * MAIR_EL1 configuration
 */
#define MAIR_ATTR_DEVICE_nGnRE    0x00    /* Device-nGnRE memory */
#define MAIR_ATTR_NORMAL_WB_RA_WA 0xFF    /* Normal Memory */

#define MAIR_EL1_VALUE ( \
    (MAIR_ATTR_DEVICE_nGnRE    << 0)  | /* AttrIdx 0 */ \
    (MAIR_ATTR_NORMAL_WB_RA_WA << 8)    /* AttrIdx 1 */ \
)

/*
 * Page Table Entry (PTE) attributes
 */
#define PTE_TYPE_BLOCK   0x1UL   /* bits[1:0] = 01 */
#define PTE_TYPE_TABLE   0x3UL   /* bits[1:0] = 11 */

#define PTE_AF           (1UL << 10)   /* Access Flag */

/* Shareability */
#define PTE_SH_OUTER     (2UL << 8)
#define PTE_SH_INNER     (3UL << 8)

/* AttrIndx (matches MAIR_EL1) */
#define PTE_ATTR_DEVICE  (0UL << 2)
#define PTE_ATTR_NORMAL  (1UL << 2)

/* Execute-never */
#define PTE_PXN          (1UL << 53)
#define PTE_UXN          (1UL << 54)

#define PTE_BLOCK_DEVICE ( \
    PTE_TYPE_BLOCK | \
    PTE_AF         | \
    PTE_SH_OUTER   | \
    PTE_ATTR_DEVICE| \
    PTE_PXN        | \
    PTE_UXN          \
)

#define PTE_BLOCK_NORMAL ( \
    PTE_TYPE_BLOCK | \
    PTE_AF         | \
    PTE_SH_INNER   | \
    PTE_ATTR_NORMAL  \
)

/*
 * SCTLR_EL1 configuration
 */
#define SCTLR_M             (_UL(1) << 0)
#define SCTLR_C             (_UL(1) << 2)
#define SCTLR_I             (_UL(1) << 12)

#endif /* _ASM_MMU_H */