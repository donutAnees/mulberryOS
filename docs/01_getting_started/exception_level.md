# Exception Levels and Execution States (ARMv8-A)
## Exception Levels (EL)

The Raspberry Pi Zero 2 W uses an ARMv8-A architecture CPU, which defines four
Exception Levels (ELs):

- EL0 – Least privileged (user mode)
- EL1 – Kernel
- EL2 – Hypervisor
- EL3 – Monitor

Every ARMv8-A CPU must implement EL0 and EL1.  
Support for EL2 and EL3 is implementation defined.

### Exception Level transitions

The current Exception Level can change only in the following cases:

- Taking an exception
- Returning from an exception (`eret`)
- Processor reset
- Exiting from Debug state
- Executing `DCPSx` in Debug state
- Executing `DRPS` in Debug state

Key rules:

- EL0 never handles exceptions
- Exceptions always move execution to the same or a higher EL
- An exception can never enter a lower Exception Level
- Each exception type has a predefined target EL, determined by the
  architecture and system configuration

## Execution States

The ARMv8-A architecture supports two execution states, which define the
instruction set and register width used by the CPU:

- AArch64 – 64-bit execution state
- AArch32 – 32-bit execution state (compatible with Armv7-A)

Execution state is **independent of privilege level**, but it is constrained by
Exception Level transitions.

### Execution state rules

- You **cannot freely switch** between AArch32 and AArch64
- Execution state changes only:
  - On processor reset
  - When changing Exception Level (exception entry or return)

Hierarchy constraints:

- If an Exception Level is using AArch32, then all lower ELs must also use
  AArch32
- If an Exception Level is using AArch64, then all higher ELs must use
  AArch64

## CurrentEL register
The `CurrentEL` system register provides the current Exception Level of the CPU.

- Bits [3:2] indicate the current EL (00 = EL0, 01 = EL1, 10 = EL2, 11 = EL3)

## Reference
- [ARM Architecture Reference Manual ARMv8, for ARMv8-A architecture profile](https://developer.arm.com/documentation/ddi0487/latest)