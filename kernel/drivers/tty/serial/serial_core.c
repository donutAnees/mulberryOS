#include <serial_core.h>
#include <types.h>

#include <stdint.h>
typedef unsigned int u32;

#define PERIPHERAL_BASE      0x3F000000
#define UART0_BASE           (PERIPHERAL_BASE + 0x201000)
#define UART0_DR             ((volatile u32*)(UART0_BASE + 0x00))

static struct uart_port *active_uart;

void uart_add_one_port(struct uart_port *port)
{
    active_uart = port;
    if (port->ops && port->ops->startup)
        port->ops->startup(port);
}

void uart_poll_putc(char c)
{
    if (!active_uart || !active_uart->ops || !active_uart->ops->poll_put_char)
        return;

    if (c == '\n')
        active_uart->ops->poll_put_char(active_uart, '\r');
    active_uart->ops->poll_put_char(active_uart, c);
}

void uart_poll_puts(const char *s)
{
    while (*s)
        uart_poll_putc(*s++);
}