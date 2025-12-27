#ifndef _SERIAL_CORE_H
#define _SERIAL_CORE_H

#include <types.h>

#ifndef __iomem
#define __iomem
#endif

enum uart_iotype {
    UPIO_MEM = 0,
};

struct uart_port;

struct uart_ops {
    int  (*startup)(struct uart_port *port);
    void (*poll_put_char)(struct uart_port *port, unsigned char ch);
};

struct uart_port {
    void __iomem            *membase;     /* MMIO base */
    unsigned int            uartclk;      /* input clock */
    unsigned int            fifosize;
    unsigned char           regshift;
    enum uart_iotype        iotype;
    const struct uart_ops   *ops;
    void                    *private_data;
};

void uart_add_one_port(struct uart_port *port);
void uart_poll_putc(char c);
void uart_poll_puts(const char *s);

#endif
