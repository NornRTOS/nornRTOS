/**
 * @file shell.c
 * nosh on the console UART beside a task that blinks the LED. Two transports, both
 * outside nosh, chosen by the board:
 *
 *   ESP32   interrupt driven: uart_onReceive() puts every byte in a queue from the
 *           receive interrupt, and read() blocks on that queue. The shell task sleeps
 *           until something is typed and costs nothing in between.
 *   AVR     polling: read() returns the byte uart_getc() has or nothing, and nosh sleeps
 *           NOSH_CFG_POLL_MS between looks (the AVR HAL has no receive interrupt yet).
 *
 * Build from the nornRTOS directory: make APP=shell BOARD=esp32 sim, then type `help`
 * (modules/nornSHELL/tools/qemu_talk.py does it from a script).
 *
 * SHELL_WITHOUT_NOSH builds the same application without the shell; the image must then
 * contain no nosh symbol and the kernel's size must be the same as without the module
 * (tools/size_check.sh). SHELL_ONLY_<CMD> (NONE, UPTIME, PS, MEM, SCHED) registers that
 * one command instead of the default set, for the cost per command in the same script.
 */

#include "rtos.h"
#include "uart.h"
#include "board.h"

#ifndef SHELL_WITHOUT_NOSH
#include "nosh.h"
#endif

#define BLINK_PRIO      2u
#define SHELL_PRIO      1u

static uint8_t _blinkStack[RTOS_STACK_DEFAULT];
static const gpio_pin_t _led = BOARD_PIN_LED;

static void blink(void *arg)
{
    (void)arg;
    gpio_output(&_led);
    for(;;)
    {
        (void)rtos_waitPeriodic(RTOS_MS(500));
        gpio_toggle(&_led);
    }
}

#ifndef SHELL_WITHOUT_NOSH
#if defined(BOARD_IS_ESP32)  &&  BOARD_IS_ESP32

/* ---- interrupt driven transport: receive interrupt -> queue -> blocking read() -------- */

/* Room for a pasted line: the hardware FIFO holds 128 bytes and the shell echoes each
   byte through the polling transmitter while more arrive. */
#define RX_QUEUE_LEN    96u
static uint8_t _rxStorage[RX_QUEUE_LEN];
static rtos_queue_t _rxQueue;

/** Called by the HAL in interrupt context for every received byte. A full queue drops
    the byte (and the queue counts it): a very long paste loses its tail. */
static void uartRx(uint8_t c, void *arg)
{
    (void)arg;
    (void)rtos_queueSendFromIsr(&_rxQueue, &c);
}

static size_t ioRead(void *buf, size_t n)
{
    (void)n;
    return rtos_queueReceive(&_rxQueue, buf, RTOS_FOREVER) ? 1u : 0u;
}

static void ioInit(void)
{
    rtos_queueInit(&_rxQueue, _rxStorage, 1, RX_QUEUE_LEN);
    uart_onReceive(uartRx, NULL);
}

#else

/* ---- polling transport ------------------------------------------------------------- */

static size_t ioRead(void *buf, size_t n)
{
    const int16_t c = uart_getc();
    (void)n;
    if(c < 0)
        return 0;
    *(uint8_t *)buf = (uint8_t)c;
    return 1;
}

static void ioInit(void)
{
}

#endif

static void ioWrite(const void *buf, size_t n)
{
    const char *p = buf;
    while(n-- > 0)
        uart_putc(*p++);
}

static const nosh_io_t _uartIo = { ioRead, ioWrite };

/** Names for `ps`, indexed by task id; the kernel itself stores none. */
static const char *const _taskNames[] = { "idle", "nosh", "blink" };
#endif  /* SHELL_WITHOUT_NOSH */

void rtos_panicHook(const rtos_panicInfo_t *pInfo)
{
    uart_puts("\r\nPANIC reason "); uart_putU32(pInfo->reason);
    uart_puts(" task "); uart_putU32(pInfo->task);
    uart_puts(" arg1 "); uart_putU32(pInfo->arg1);
    uart_puts(" arg2 "); uart_putU32(pInfo->arg2);
    uart_puts("\r\n");
    uart_flush();
}

int main(void)
{
    uart_init(115200);
    uart_puts("\r\nnornRTOS shell example\r\n");
    rtos_init();
    rtos_createTask(BLINK_PRIO, blink, NULL, _blinkStack, sizeof(_blinkStack));
#ifndef SHELL_WITHOUT_NOSH
    ioInit();
#if defined(SHELL_ONLY_NONE) || defined(SHELL_ONLY_UPTIME) || defined(SHELL_ONLY_PS) \
    || defined(SHELL_ONLY_MEM) || defined(SHELL_ONLY_SCHED)
    NOSH_ENABLE(&_uartIo, SHELL_PRIO, RTOS_STACK_DEFAULT);
#ifdef SHELL_ONLY_UPTIME
    nosh_add(&nosh_cmd_uptime);
#endif
#ifdef SHELL_ONLY_PS
    nosh_add(&nosh_cmd_ps);
#endif
#ifdef SHELL_ONLY_MEM
    nosh_add(&nosh_cmd_mem);
#endif
#ifdef SHELL_ONLY_SCHED
    nosh_add(&nosh_cmd_sched);
#endif
#else
    NOSH_ENABLE_DEFAULT(&_uartIo, SHELL_PRIO, RTOS_STACK_DEFAULT);
#endif
    nosh_setTaskNames(_taskNames, sizeof _taskNames / sizeof *_taskNames);
#endif
    rtos_start();
    return 0;
}
