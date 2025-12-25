# Booting
Unlike traditional x86 architectures, the boot process on a Raspberry Pi relies heavily on firmware and the GPU rather than the CPU.

## Key Files
1. bootcode.bin
2. fixup.dat
3. start.elf

## Boot Process
The GPU is responsible for starting the boot process, unlike x86 systems which use the CPU in real mode to begin booting. 

1. ROM Bootloader: The GPU executes the bootloader stored in the Pi’s ROM, which locates the SD card.

2. Load Bootcode: On models prior to the RPi4, the bootloader loads `bootcode.bin` from the SD card. This step is skipped on the RPi4 since the ROM already includes `bootcode.bin`.

3. Load Firmware: `bootcode.bin` loads `start.elf` which is the executable firmware, reads `config.txt` and `cmdline.txt`, uses `fixup.dat` which provides hardware-specific configuration and memory information and is used to patch the device tree before the ARM CPU starts executing the kernel.

4. Finally it loads the `kernel*.img` and starts the ARM CPU execution of the kernel.

## Kernel Entry Point

When `start.elf` loads our kernel image (`kernel8.img` for 64-bit), it places the kernel at a specific memory address (typically `0x80000` for ARM64) and begins execution. The bootloader firmware passes the following information through registers:

- **x0**: Device Tree Blob (DTB) pointer - Contains hardware description
- **x4**: Kernel runtime base address - Where the kernel can be loaded in memory

### boot.S - Assembly Entry Point

Our kernel starts with `boot.S`. This assembly file is placed in the `.text.boot` section to ensure it's the first code that executes.

**Key responsibilities of boot.S:**

1. **Stack Setup**: Establishes a temporary 16KB stack placed immediately after the kernel image. The ARM64 stack grows downward, so the stack pointer (SP) is set to the top of this region.

2. **BSS Clearing**: Zeros out the `.bss` section which contains uninitialized global and static variables. This is critical because C code expects these to be zero.

3. **Jump to C Code**: Branches to `kernel_main()` to begin C code execution.

The boot code uses **position-independent** addressing - it doesn't hardcode memory addresses. Instead, it uses the `adr` instruction to calculate offsets from the program counter and the x4 register (runtime base) to find actual memory addresses.

### linker.ld - Memory Layout Script

The linker script defines how our kernel binary is organized in memory. It specifies:

**Memory Sections (in order):**

1. **.text.boot**: Boot assembly code (entry point)
2. **.text**: Compiled C functions
3. **.rodata**: Read-only data (string literals, constants) - 8-byte aligned
4. **.data**: Initialized global/static variables - 8-byte aligned
5. **.bss**: Uninitialized global/static variables - 16-byte aligned

**Important Symbols:**
- `_start`: Beginning of kernel
- `_end`: End of kernel image
- `__bss_start`: Start of BSS section
- `__bss_end`: End of BSS section
``

**Alignment Requirements:**
- **8-byte**: Data and rodata sections (standard alignment)
- **16-byte**: BSS section and stack (SIMD/NEON register size)