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

SCTLR_ELx
- M - Enable Memory Management Unit (MMU).
- C - Enable for data and unified caches.
- EE - Endianness of translation table walks.

TTBR0_ELx and TTBR1_ELx
- BADDR - Physical address (PA) (or intermediate physical address, IPA, for EL0/EL1) of start of translation table.
- ASID - The Address Space Identifier for Non-Global translations.

TCR_ELx
- PS/IPS - Size of PA or IPA space, the maximum output address size.
- TnSZ - Size of address space covered by table.
- TGn - Granule size.
- SH/IRGN/ORGN - Cacheability and shareability to be used by MMU table walks.
- EPDn - Disabling of table walks to a specific table.

MAIR_ELx
- Attr - Controls the Type and cacheability in Stage 1 tables.

Reference: [AArch64 memory management Guide](https://developer.arm.com/documentation/101811/0105)