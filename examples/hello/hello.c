/**
 * @file hello.c
 * The smallest nornRTOS application: one task that prints a line once a second and
 * toggles the board's LED, the idle hook counting idle loops. Build it from the nornRTOS
 * directory (make APP=hello BOARD=esp32 sim) or against the binary SDK (make in
 * examples/hello of the SDK).
 */

#include "rtos.h"
#include "uart.h"
#include "board.h"

static uint8_t _stack[RTOS_STACK_DEFAULT];
static const gpio_pin_t _led = BOARD_PIN_LED;
static volatile uint32_t _idleLoops;

void rtos_idleHook(void)
{
    ++ _idleLoops;
}

static void task(void *arg)
{
    uint32_t n = 0;
    (void)arg;
    gpio_output(&_led);
    for(;;)
    {
        (void)rtos_waitPeriodic(RTOS_MS(1000));
        gpio_toggle(&_led);
        uart_puts("hello from nornRTOS, second ");
        uart_putU32(++n);
        uart_puts(", idle loops ");
        uart_putU32(_idleLoops);
        uart_puts("\r\n");
    }
}

int main(void)
{
    uart_init(115200);
    uart_puts("\r\nnornRTOS hello\r\n");
    rtos_init();
    rtos_createTask(1, task, NULL, _stack, sizeof(_stack));
    rtos_start();
    return 0;
}
