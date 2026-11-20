/**
 * @file shell.c
 * nosh on the console UART beside a task that blinks the LED, and a program (`work`) the
 * console starts and stops. The transport is nosh's ready made one for the console UART
 * (nosh_enableUart, modules/nornSHELL/src/nosh_uart.c): the UART's receive interrupt into
 * a stream buffer on both boards (UART_CFG_RX_STREAM in rtos_config.h); an application
 * with another transport writes its own nosh_io_t instead.
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
#define WORK_PRIO       3u      /* the program's slot: nothing is created in it at start */

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
/** Names for `ps`, indexed by task id; the kernel itself stores none. The program's task
    takes its name from the program table. */
static const char *const _taskNames[] = { "idle", "nosh", "blink" };

/* ---- a program: `run work` starts it, `stop work` asks it to end, `wait work` reaps,
        `restart work` does all three ----------------------------------------------- */

/** Prints a line every second until told to stop; the stack is ours and is lent to each
    run. A program ends itself: here by returning, rtos_exit() would do the same. It
    writes to the UART itself, not through nosh_puts (the shell task's output). */
static void work(void *arg)
{
    uint32_t n = 0;
    (void)arg;
    while(!nosh_stopRequested())
    {
        rtos_delay(RTOS_MS(1000));
        uart_puts("work: ");
        uart_putU32(++n);
        uart_puts("\r\n");
    }
    uart_puts("work: stopping\r\n");
}

RTOS_STACK(_workStack, RTOS_STACK_DEFAULT);
static const nosh_program_t _programs[] =
{
    { "work", WORK_PRIO, work, NULL, _workStack, sizeof _workStack },
};
#endif  /* SHELL_WITHOUT_NOSH */

int main(void)
{
    uart_init(UART_BAUD_DEFAULT);
    uart_puts("\r\nnornRTOS shell example\r\n");
    rtos_init();
    RTOS_TASK(BLINK_PRIO, blink, NULL, RTOS_STACK_DEFAULT);
#ifndef SHELL_WITHOUT_NOSH
#if defined(SHELL_ONLY_NONE) || defined(SHELL_ONLY_UPTIME) || defined(SHELL_ONLY_PS) \
    || defined(SHELL_ONLY_MEM) || defined(SHELL_ONLY_SCHED)
    nosh_uartInit();
    NOSH_ENABLE(&nosh_uartIo, SHELL_PRIO, RTOS_STACK_DEFAULT);
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
    (void)nosh_enableUart(SHELL_PRIO);          /* the console UART and the default set */
    nosh_add(&nosh_cmd_run);
    nosh_add(&nosh_cmd_stop);
    nosh_add(&nosh_cmd_wait);
    nosh_add(&nosh_cmd_restart);
    nosh_setPrograms(_programs, sizeof _programs / sizeof *_programs);
#endif
    nosh_setTaskNames(_taskNames, sizeof _taskNames / sizeof *_taskNames);
#endif
    rtos_start();
    return 0;
}
