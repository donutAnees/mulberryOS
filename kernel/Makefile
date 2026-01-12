# ============================================================
# Toolchain
# ============================================================

CROSS_COMPILE ?= $(HOME)/opt/cross/bin/aarch64-elf-
CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

# ============================================================
# Flags
# ============================================================

CFLAGS  := -Wall -Werror -O2
CFLAGS  += -ffreestanding -nostdlib -nostartfiles
CFLAGS  += -mgeneral-regs-only -fno-common
CFLAGS  += -Iinclude
CFLAGS  += -Iarch/arm64/include

LDFLAGS := -nostdlib -T arch/arm64/kernel/linker.ld

# ============================================================
# Build output
# ============================================================

BUILD := build

KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_IMG := $(BUILD)/kernel8.img

# ============================================================
# Sources
# ============================================================

BOOT_S := arch/arm64/boot/boot.S

ASM_SRC := \
	arch/arm64/kernel/exceptions.S

C_SRC := \
	init/main.c \
	arch/arm64/kernel/exception_handler.c \
	drivers/tty/serial/serial_core.c \
	drivers/tty/serial/amba-pl011.c \
	kernel/irq/irq.c \
	kernel/irq/irq_chip.c 

# ============================================================
# Objects
# ============================================================

BOOT_O := $(BUILD)/arch/arm64/boot/boot.o
ASM_OBJ := $(patsubst %.S,$(BUILD)/%.o,$(ASM_SRC))
C_OBJ  := $(patsubst %.c,$(BUILD)/%.o,$(C_SRC))
ALL_OBJ := $(BOOT_O) $(ASM_OBJ) $(C_OBJ)

# ============================================================
# Targets
# ============================================================

.PHONY: all clean

all: $(KERNEL_IMG)
	@echo ""
	@echo "=========================================="
	@echo "  Build complete!"
	@echo "  Output: $(KERNEL_IMG)"
	@echo "=========================================="

# ============================================================
# Link + image
# ============================================================

$(KERNEL_IMG): $(KERNEL_ELF)
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(ALL_OBJ)
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(LD) $(LDFLAGS) -o $@ $^
	@$(OBJDUMP) -t $@ | sort > $(BUILD)/kernel.map
	@echo "  Generated symbol map: $(BUILD)/kernel.map"

# ============================================================
# Compile rules
# ============================================================

$(BUILD)/arch/arm64/boot/boot.o: $(BOOT_S)
	@echo "  AS      $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S
	@echo "  AS      $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.c
	@echo "  CC      $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# ============================================================
# Clean
# ============================================================

clean:
	@echo "  CLEAN"
	@rm -rf $(BUILD)