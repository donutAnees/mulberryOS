/*
 * ARM AMBA PrimeCell PL011 Serial Port Register Definitions
 * https://developer.arm.com/documentation/ddi0183/g/programmers-model/summary-of-registers?lang=en
 */

#ifndef _AMBA_SERIAL_H
#define _AMBA_SERIAL_H

/* PL011 Base Address */
// TODO: Implement DTB parsing to get base addresses dynamically
# define PL011_UART0_BASE      0x3F201000  /* Raspberry Pi Zero 2 W (BCM2837) - ARM CPU address */

/* PL011 UART Clock and Baud Rate Calculation Macros */
#define PL011_UART_CLOCK_HZ 48000000U
#define PL011_DEFAULT_BAUD 115200U
#define PL011_IBRD(uartclk, baud) ((uartclk) / (16U * (baud)))
#define PL011_FBRD(uartclk, baud) \
    ((((uartclk) % (16U * (baud))) * 64U + ((baud) / 2U)) / (baud))


/*
 * PL011 UART Register Offsets.
 * The base address of the UART is not fixed. 
 * The offset of each register from the base address is fixed.
 */
# define UARTDR        0x00  /* Data Register - RW */
# define UARTRSR       0x04  /* Receive Status Register - RW */
# define UARECR        0x04  /* Error Clear Register - RW */
# define UARTFR        0x18  /* Flag Register - RO */
# define UARTILPR      0x20  /* IrDA Low-Power Register - RW */
# define UARTIBRD      0x24  /* Integer Baud Rate Register - RW */
# define UARTFBRD      0x28  /* Fractional Baud Rate Register - RW */
# define UARTLCR_H     0x2C  /* Line Control Register - RW */
# define UARTCR        0x30  /* Control Register - RW */
# define UARTIFLS      0x34  /* Interrupt FIFO Level Select Register - RW */
# define UARTIMSC      0x38  /* Interrupt Mask Set/Clear Register - RW */
# define UARTRIS       0x3C  /* Raw Interrupt Status Register - RO */
# define UARTMIS       0x40  /* Masked Interrupt Status Register - RO */
# define UARTICR       0x44  /* Interrupt Clear Register - WO */
# define UARTDMACR     0x48  /* DMA Control Register - RW */

#endif /* _AMBA_SERIAL_H */