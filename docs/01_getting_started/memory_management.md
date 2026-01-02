# Memory Management
Memory management describes how access to memory in a system is controlled. The hardware performs memory management every time that memory is accessed by either the OS or applications. Memory management is a way of dynamically allocating regions of memory to applications.

## Virtual Address and Physical Address
Benefits:
- OS can control the view of memory presented to each application (what memory regions it can access)
- Show fragmented physical memory as a contiguous block to applications
- Application develop need not worry about physical memory layout

### Translation Table
The mapping between virtual and physical addresses is stored in a data structure called a translation table.

### Physical Addresses
AArch64 also has multiple physical address spaces (PAS):
- Non-secure PAS
- Secure PAS
- Realm PAS (Armv9-A only)
- Root PAS (Armv9-A only)

The following list shows the Security states with its corresponding virtual address mapping destinations:
- Non-secure state: virtual addresses can only map to Non-secure physical addresses.
- Secure state: virtual addresses can map to Secure or Non-secure physical addresses.
- Realm state: virtual addresses can map to Realm or Non-secure physical addresses.
- Root state: virtual address can map to any physical address space.

In virtualization, address translation happens in two stages.
Stage 1, controlled by the guest OS, translates virtual addresses to Intermediate Physical Addresses (IPAs), which the OS treats as physical memory.
Stage 2, controlled by the hypervisor, translates those IPAs to real physical addresses, enforcing isolation between virtual machines.

### Size of virtual addresses
Virtual addresses are stored in a 64-bit format.

The EL0/EL1 virtual address space is conventionally split into user space and kernel space. Kernel space and user space have separate translation tables and this means that their mappings can be kept separate.

If HCR_EL2.E2H is set, EL2 acts as a host OS and, like EL1, gains separate user and kernel regions.

Each region can be up to 55 bits in size, but the actual size is configurable. The TCR_ELx.TnSZ fields determine how much of the virtual address space is used at each Exception Level, with TCR_EL1 controlling the EL0/EL1 address space.

The virtual address size is encoded as:
`virtual address size in bytes = 264-TCR_ELx.TnSZ`

For example, imagine that the OS kernel needs 1GB of address space (30-bit address size) for its kernel space. If the OS sets T1SZ to 34, then only the translation table entries to describe 1GB are created, as 64 - 34 = 30.

### Single VA range
- TTBR0_ELx is used for all virtual addresses.
If a translation regime supports one VA range:
The maximum IA width is 56 bits.
- The VA range always starts at 0x0000_0000_0000_0000
- Maximum VA range depends on configured VA size:
    - 48-bit VA: 0x0000_0000_0000_0000 – 0x0000_FFFF_FFFF_FFFF
    - 52-bit VA: 0x0000_0000_0000_0000 – 0x000F_FFFF_FFFF_FFFF
    - 56-bit VA: 0x0000_0000_0000_0000 – 0x00FF_FFFF_FFFF_FFFF

### Two VA ranges (Lower + Upper)
TTBR selection is required.
- TTBR0_ELx is used for the lower VA range.
- TTBR1_ELx is used for the upper VA range.
The VA space is split into two non-contiguous regions.
The maximum IA width is 55 bits.
- IA bit[55] determines one of the following VA ranges:
    - If IA bit[55] is 0, the lower VA range is used.
    - If IA bit[55] is 1, the upper VA range is used.
- The maximum lower VA range is one of the following:
    - If the maximum VA size is 48 bits, then the maximum VA range is 0x0000_0000_0000_0000 to 0x0000_FFFF_FFFF_FFFF.
    - If the maximum VA size is 52 bits, then the maximum VA range is 0x0000_0000_0000_0000 to 0x000F_FFFF_FFFF_FFFF.
    - If the maximum VA size is 55 bits, then the maximum VA range is 0x0000_0000_0000_0000 to 0x007F_FFFF_FFFF_FFFF.
- The maximum upper VA range is one of the following:
    - If the maximum VA size is 48 bits, then the maximum VA range is 0xFFFF_0000_0000_0000 to 0xFFFF_FFFF_FFFF_FFFF.
    - If the maximum VA size is 52 bits, then the maximum VA range is 0xFFF0_0000_0000_0000 to 0xFFFF_FFFF_FFFF_FFFF.
    - If the maximum VA size is 55 bits, then the maximum VA range is 0xFF80_0000_0000_0000 to 0xFFFF_FFFF_FFFF_FFFF.

### Size of Physical addresses
Physical address size is implementation-defined

Maximum supported size is 56 bits. Actual size is reported by ID_AA64MMFR0_EL1.

Most Arm Cortex-A CPUs support 40-bit or 44-bit physical addresses. Armv8.0-A supports up to 48-bit physical addresses

### Size of intermediate physical addresses
VTCR_EL2.T0SZ controls the size. The maximum size that can be configured is the same as the physical address size that is supported by the processor. 

## Address Space Identifiers (ASIDs)
All applications appear to run in the same user virtual address space, but each requires different VA -> PA mappings. Ideally, mappings from different applications should coexist in the TLB to avoid flushing on context switches.

Address Space Identifiers (ASIDs) solve this problem in AArch64

Translation table entries can be marked:
- Global (G) -> valid for all applications (typically kernel mappings)
- Non-Global (nG) -> valid only for one application (user mappings)

Global mappings are shared across all processes and are not ASID-tagged, whereas Non-Global mappings are tagged with an ASID in the TLB.

### Register used
The ASID is stored in `TTBRn_EL1` along with the base of the translation table. Typically, TTBR0_EL1 is used for user space, so updating it switches both the ASID and page tables.

When the CPU looks up a translation in the TLB:
1. It matches the virtual address
2. It checks whether the entry is:
    - Global (G) → always valid
    - Non-Global (nG) → ASID must match
3. For non-global entries:
    - The TLB entry’s ASID is compared with the current ASID
    - If they match → entry is used
    - If they don’t → entry is ignored

On Armv8.0-A, ASIDs and VMIDs need not be consistent across processors, preventing cross-CPU TLB reuse.This is addressed in the later armv8.2-A architecture update.

## Memory Management Unit (MMU)
The MMU translates software virtual addresses to physical addresses using 
- TLBs to cache recent translations 
- a Table Walk Unit to read translation tables from memory when a cached entry is not found in the TLB.

A virtual address must be translated to a physical address before a memory access, including cache accesses, because Armv6 and later processors use physically tagged caches, requiring address translation before cache lookup.

### Table entry
The translation tables work by dividing the virtual address space into equal-sized blocks and by providing one entry in the table per block. Each entry contains the address of a corresponding block of physical memory and the attributes to use when accessing the physical address.

Virtual Memory Block -> Translation Table Entry (Table for that Process) -> Physical Memory Block

### Table lookup
#### Single Level Translation
In a single-level translation, the MMU uses one translation table to map virtual addresses to physical addresses.

When the MMU receives a virtual address to translate, it uses the following process:
1. The MMU divides the virtual address into parts:
   - An index into the translation table at each level
   - A page offset within the final block
2. The MMU uses the index to look up the corresponding entry in the translation table at each level
3. The MMU retrieves the physical address from the final entry and combines it with the page offset to form the complete physical address.

#### Multi-Level Translation
In a multi-level translation, the MMU uses multiple translation tables to map virtual addresses to physical addresses.

When the MMU receives a virtual address to translate, it uses the following process:
1. The MMU divides the virtual address into parts:
   - An index into the translation table at each level
   - A page offset within the final block
2. The MMU uses the index to look up the corresponding entry in the translation table at each level, starting from the root table
3. The MMU retrieves the address of the next level table from the current entry and uses it to look up the next entry
4. This process continues until the final entry is reached
5. The MMU retrieves the physical address from the final entry and combines it with the page offset to form the complete physical address.

MMU determines whether the entry points to a block or next level table by checking the type field in the entry.

In Armv8-A, the maximum number of levels is six, and the levels are numbered -2 to 3.

The characteristics of large and small blocks are as follows:
- Large blocks require fewer levels of reads to translate than small blocks. Plus, large blocks are more efficient to cache in the TLBs.
- Small blocks give software fine-grain control over memory allocation. However, small blocks are less efficient to cache in the TLBs. Caching is less efficient because small blocks require multiple reads through the levels to translate.
- To manage this trade-off, an OS must balance the efficiency of using large mappings against the flexibility of using smaller mappings for optimum performance.

+--------+--------+--------+--------+--------+--------+--------+--------+
|63    56|55    48|47    40|39    32|31    24|23    16|15     8|7      0|
+--------+--------+--------+--------+--------+--------+--------+--------+
 |                 |         |         |         |         |
 |                 |         |         |         |         v
 |                 |         |         |         |   [11:0]  in-page offset
 |                 |         |         |         +-> [20:12] L3 index
 |                 |         |         +-----------> [29:21] L2 index
 |                 |         +---------------------> [38:30] L1 index
 |                 +-------------------------------> [47:39] L0 index
 +-------------------------------------------------> [63] TTBR0/1

Page offset = 12 bits → 4 KB page

#### AArch64 (4 KB granule) address translation breakdown
1. Bits [11:0] - Offset within a 4 KB page.
2. Bits [20:12] — Level 3 (L3) - Index into an L3 page table (512 entries), each mapping a 4 KB page.
3. Bits [29:21] — Level 2 (L2) - Index into an L2 table (512 entries). Each entry points to an L3 table or maps a 2 MB block.
4. Bits [38:30] — Level 1 (L1) - Index into an L1 table (512 entries). Each entry points to an L2 table or maps a 1 GB block.
5. Bits [47:39] — Level 0 (L0) - Index into the L0 table (512 entries). Each entry points to an L1 table or maps a 512 GB block.

The bits [63] determine whether TTBR0_ELx or TTBR1_ELx is used for translation.

Example:
Suppose we have 2GB of RAM then we can create mappings as follows:
- L0 table with 1 entry pointing to an L1 table (covers 512GB)
- L1 table with 2 entries, each pointing to an L2 table (covers 1GB each) - total 2GB

We can stop here as we have mapped the entire RAM. Unless finer granularity is needed, we do not need to create L3/L4 tables.

### Translation Granule
The starting level of address translation is determined by:
- The translation granule size (4KB / 16KB / 64KB)
- The virtual address size, configured by TCR_ELx.TnSZ. TnSZ defines the number of unused top bits of the virtual address. 

`Virtual address size = 64 − TnSZ bits`

Each translation level indexes a fixed set of virtual address bits, based on the granule size. If the bits required to index a given level are all zero or absent, that level is skipped.

Example with 4KB granule:
T0SZ = 32 → 32-bit VA (bits [31:0])
→ Level 0 index bits [47:39] are zero
→ Translation starts at Level 1

T0SZ = 34 → 30-bit VA (bits [29:0])
→ No bits for Level 0 or Level 1 indexing
→ Translation starts at Level 2

As the virtual address space shrinks, fewer page-table levels are needed. The same concept applies to 16KB and 64KB granules, though the indexing bits differ.

## Registers Used
Address translation is controlled by a combination of system registers:

SCTLR_ELx - System Control Register
- M - Enable Memory Management Unit (MMU).
- C - Enable for data and unified caches.
- EE - Endianness of translation table walks.

TTBR0_ELx and TTBR1_ELx - Translation Table Base Registers
- BADDR - Physical address (PA) (or intermediate physical address, IPA, for EL0/EL1) of start of translation table.
- ASID - The Address Space Identifier for Non-Global translations.

TCR_ELx - Translation Control Register
- PS/IPS - Size of PA or IPA space, the maximum output address size.
- TnSZ - Size of address space covered by table.
- TGn - Granule size.
- SH/IRGN/ORGN - Cacheability and shareability to be used by MMU table walks.
- EPDn - Disabling of table walks to a specific table.

MAIR_ELx - Memory Attribute Indirection Register
- Attr - Controls the Type and cacheability in Stage 1 tables.

## Implementation Notes  
All the required bit values are taken from the official docs, refer them.

## TCR_EL1 Configuration
```
PS   [34:32]  – Physical address size
TG1   [31:30]  – TTBR1 granule size
SH1   [29:28]  – TTBR1 shareability
ORGN1 [27:26]  – TTBR1 outer cacheability
IRGN1 [25:24]  – TTBR1 inner cacheability
EPD1  [23]     – Disable TTBR1 table walks
A     [22]     – ASID select
T1SZ  [21:16]  – TTBR1 virtual address size offset
TG0   [15:14]  – TTBR0 granule size
SH0   [13:12]  – TTBR0 shareability
ORGN0 [11:10]  – TTBR0 outer cacheability
IRGN0 [9:8]    – TTBR0 inner cacheability
EPD0  [7]      – Disable TTBR0 table walks
RES   [6]      – Reserved [Not used, set to 0]
T0SZ  [5:0]    – TTBR0 virtual address size offset
```

### Virtual address size - T1SZ [21:16] and T0SZ [5:0]
- We will be utilizing just 4 levels of translation tables with 4KB granules.
- `TCR_EL1.T0SZ = 16` and `TCR_EL1.T1SZ = 16`
- Virtual address size: `VA size = 64 − TnSZ = 48 bits`
- This provides 256 TB of virtual address space for both user (TTBR0) and kernel (TTBR1).
- User addresses have bits 63:48 set to 0 while the kernel addresses have the same bits set to 1. TTBRx selection is given by bit 63 of the virtual address.
- (1 << 48) - 1 = 0xffffffffffff, this will be the maximum virtual address that can be used by the user space.
- kernel virtual addresses will start from 0xffff000000000000 to 0xffffffffffffffff

With this configuration and a 4KB granule size, translation begins at **level 1**, and level 0 is not used.

### Translation granule - TG0 [15:14] and TG1 [31:30]
- Use 4KB translation granules for both user and kernel address spaces.
- Configure:
  - `TCR_EL1.TG0 = 0b00` (4KB)
  - `TCR_EL1.TG1 = 0b10` (4KB)

### Physical address size - PS [34:32]
- Read `ID_AA64MMFR0_EL1.PARange - bits [3:0]` to determine the implemented physical address size.
- Configure `TCR_EL1.PS` to match the implemented PA size.
- The configured Output Address (OA) size must not exceed the implemented PA size.

### Cacheability and Shareability
The following `TCR_EL1` fields must be configured to obtain defined memory behavior:

- `IRGN0 - [9:8]`, `IRGN1 - [25:24]` — Inner cacheability related to TTBR0 and TTBR1
    - `0b01` - Normal Memory, Inner Write-Back, Write-Allocate
- `ORGN0 - [11:10]`, `ORGN1 - [27:26]` — Outer cacheability related to TTBR0 and TTBR1
    - `0b01` - Normal Memory, Outer Write-Back, Write-Allocate
- `SH0 - [13:12]`, `SH1 - [29:28]` — Shareability related to TTBR0 and TTBR1
 - `0b11` - Inner Shareable

Minimum required policy:
- Normal memory: Inner-shareable, cacheable
- Device memory: Non-shareable, non-cacheable

### Translation table walk disable - EPD0 [7] and EPD1 [23]
- Set both `EPD0` and `EPD1` to `0` to enable table walks for both TTBR0 and TTBR1.

### ASID Bit - A[22]
- Selects whether TTBR0_EL1 or TTBR1_EL1 defines the ASID. 
- Set `A = 0` to use the ASID from `TTBR0_EL1`.

## MAIR_EL1
`MAIR_EL1` defines the memory attributes referenced by translation table descriptors. Provides the memory attribute encodings corresponding to the possible AttrIndx values in a Long-descriptor format translation table entry for stage 1 translations at EL1.

At minimum, the following attributes must be defined:
- AttrIdx 0 [7:0] — Device memory
    (Device-nGnRE) - 0b00000000 
- AttrIdx 1 [15:8] — Normal memory
  (Inner/Outer Write-Back, Write-Allocate) - 0b11111111

All translation table entries must reference a valid attribute index.

## Translation Table Base Registers
### TTBR0_EL1
- Holds the base address of the user-space translation tables.
- Base address must be aligned to the translation granule size (4KB).

### TTBR1_EL1
- Holds the base address of the kernel-space translation tables.
- Base address must be aligned to the translation granule size (4KB).

We can make them both point to the same translation table for now since we are not implementing user space yet.

## Translation Table Descriptor Format

### Descriptor validity
- **bit[0]
  - `0` → Invalid descriptor
  - `1` → Valid descriptor

If an invalid descriptor is encountered during a translation table walk, a **Translation fault** is generated at the current lookup level.

### Descriptor type (bit[1])
- **Lookup levels 0–2**
  - `0` → Block descriptor
  - `1` → Table descriptor
- **Lookup level 3**
  - `0` → Reserved (treated as invalid)
  - `1` → Page descriptor

### Output Address (OA) size supported by descriptors
For VMSAv8-64 translations, the maximum OA size depends on the translation granule and architectural features:

- **4KB or 16KB granules**
  - `TCR_ELx.DS = 0` → 48-bit OA
  - `TCR_ELx.DS = 1` → 52-bit OA

- **64KB granule**
  - FEAT_LPA not implemented → 48-bit OA
  - FEAT_LPA implemented → 52-bit OA

## SCTLR_EL1 
The following `SCTLR_EL1` fields must be configured to enable address translation:

- `M` — Enable MMU
- `C` — Enable data cache
- `I` — Enable instruction cache

These bits must be set **after** translation tables and control registers are fully configured.

## Required Synchronization
To ensure defined behavior, the following barriers are required:
- A `DSB` after setting up translation tables and control registers.
- An `ISB` after writing:
  - `TCR_EL1`
  - `TTBR0_EL1`
  - `TTBR1_EL1`
  - `SCTLR_EL1`

Reference: [AArch64 memory management Guide](https://developer.arm.com/documentation/101811/0105)