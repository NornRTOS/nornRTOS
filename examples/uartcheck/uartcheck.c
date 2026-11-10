/**
 * @file uartcheck.c
 * Serial link check (see app.mk): no tasks, just the UART. Prints the UART setting, 200
 * lines "Lnnn <pattern>", then "END lines=200", and idles.
 */

#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"

static const char _pattern[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int main(void)
{
    unsigned i;

    uart_init(115200);
#if defined(UARTCHECK_U2X) && UARTCHECK_U2X
    UCSR0A = _BV(U2X0);
    UBRR0 = 16;
#endif
    _delay_ms(300);                     /* let the USB side settle after the reset */
    uart_puts("\r\nuartcheck UBRR0=");
    uart_putU32(UBRR0);
    uart_puts(" U2X=");
    uart_putU32((UCSR0A & _BV(U2X0)) ? 1u : 0u);
    uart_puts("\r\n");
    for(i=0; i<200u; ++i)
    {
        uart_putc('L');
        uart_putc((char)('0' + i / 100u));
        uart_putc((char)('0' + i / 10u % 10u));
        uart_putc((char)('0' + i % 10u));
        uart_putc(' ');
        uart_puts(_pattern);
        uart_puts("\r\n");
    }
    uart_puts("END lines=200\r\n");
    for(;;)
        ;
}
