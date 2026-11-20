#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the shell example: the blink task at 2, the shell at 1, slot 3
 * for the `work` program started from the console, the default options so that the
 * diagnostics commands have something to show.
 */

#define RTOS_CFG_NUM_TASKS      4
#define RTOS_CFG_STACK_CHECK    2
#define RTOS_CFG_EVGROUPS       0
#define RTOS_CFG_HAL_HOOKS      1       /* panics and stack warnings on the console, hal/hooks.c */
#define RTOS_CFG_DYNAMIC_TASKS  1       /* run, stop, wait and restart in nosh need it */
#define RTOS_CFG_RUNTIME_STATS  2       /* run time and switch count: ps and sched show them */
/* The console's receive stream on both boards: the UART interrupt writes it, the shell
   sleeps on it. Guarded so that -DUART_CFG_RX_STREAM=0 on the command line gives the
   queue (ESP32) or polling (AVR) transport for a comparison. */
#ifndef UART_CFG_RX_STREAM
# define UART_CFG_RX_STREAM     1
#endif
#define RTOS_CFG_STREAMS        1
/* Everything not set here is the kernel's default (kernel/rtos_types.h). */

#endif  /* RTOS_CONFIG_INCLUDED */
