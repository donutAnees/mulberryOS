/*
 * Basic AMBA PL011 UART driver
 *
 * This is a simplified UART driver for ARM PL011 UART controllers,
 * targeting the Raspberry Pi Zero 2 W.
 * Only basic polling and write (transmit) support is implemented.
 *
 * Reference:
 * https://developer.arm.com/documentation/ddi0183/g/?lang=en
 *
 */   

#include <amba/serial.h>
#include <serial_core.h>
#include <types.h>
#include <container_of.h> 

/* Register indices */
enum {
    REG_DR,
    REG_RSR,
    REG_ECR,
    REG_FR,
    REG_ILPR,
    REG_IBRD,
    REG_FBRD,
    REG_LCR_H,
    REG_CR,
    REG_IFLS,
    REG_IMSC,
    REG_RIS,
    REG_MIS,
    REG_ICR,
    REG_DMACR,
    REG_ARRAY_SIZE
};

/* Register offsets */
static const uint16_t pl011_std_offsets[REG_ARRAY_SIZE] = {
    [REG_DR]    = UARTDR,
    [REG_RSR]   = UARTRSR,
    [REG_ECR]   = UARECR,
    [REG_FR]    = UARTFR,
    [REG_ILPR]  = UARTILPR,
    [REG_IBRD]  = UARTIBRD,
    [REG_FBRD]  = UARTFBRD,
    [REG_LCR_H] = UARTLCR_H,
    [REG_CR]    = UARTCR,
    [REG_IFLS]  = UARTIFLS,
    [REG_IMSC]  = UARTIMSC,
    [REG_RIS]   = UARTRIS,
    [REG_MIS]   = UARTMIS,
    [REG_ICR]   = UARTICR,
    [REG_DMACR] = UARTDMACR,
};

struct uart_pl011_port {
    struct uart_port port;
    uintptr_t        base;
    const uint16_t  *offsets;
};

static struct uart_pl011_port pl011_uart0 = {
    .base    = PL011_UART0_BASE,
    .offsets = pl011_std_offsets,
    .port = {
        .membase  = (void *)PL011_UART0_BASE,
        .uartclk  = PL011_UART_CLOCK_HZ,
        .fifosize = 16,
        .regshift = 0,
        .iotype   = UPIO_MEM,
    },
};

/* MMIO helpers */
static inline void pl011_write(struct uart_pl011_port *p,
                               unsigned int reg, uint32_t val)
{
    volatile uint32_t *addr =
        (volatile uint32_t *)(p->base + p->offsets[reg]);
    *addr = val;
}

static inline uint32_t pl011_read(struct uart_pl011_port *p,
                                  unsigned int reg)
{
    volatile uint32_t *addr =
        (volatile uint32_t *)(p->base + p->offsets[reg]);
    return *addr;
}

/* uart_ops callbacks (serial_core-facing) */
static int pl011_startup(struct uart_port *port)
{
    struct uart_pl011_port *uap =
        container_of(port, struct uart_pl011_port, port);

    pl011_write(uap, REG_CR, 0);
    pl011_write(uap, REG_ICR, 0x7FF);

    pl011_write(uap, REG_IBRD,
        PL011_IBRD(port->uartclk, PL011_DEFAULT_BAUD));
    pl011_write(uap, REG_FBRD,
        PL011_FBRD(port->uartclk, PL011_DEFAULT_BAUD));

    pl011_write(uap, REG_LCR_H, (3 << 5) | (1 << 4));
    pl011_write(uap, REG_CR, (1 << 0) | (1 << 8) | (1 << 9));

    return 0;
}

static void pl011_poll_put_char(struct uart_port *port, unsigned char ch)
{
    struct uart_pl011_port *uap =
            container_of(port, struct uart_pl011_port, port);

    while (pl011_read(uap, REG_FR) & (1 << 5));

    pl011_write(uap, REG_DR, ch);

}

static const struct uart_ops pl011_uart_ops = {
    .startup       = pl011_startup,
    .poll_put_char = pl011_poll_put_char,
};

void pl011_register(void)
{
    pl011_uart0.port.ops = &pl011_uart_ops;
    uart_add_one_port(&pl011_uart0.port);
}
