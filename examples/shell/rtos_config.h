#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the shell example: the blink task at 2, the shell at 1, the
 * default options so that the diagnostics commands have something to show.
 */

#define RTOS_CFG_NUM_TASKS      4
#define RTOS_CFG_TICK_HZ        1000ul
#define RTOS_CFG_TICK_BITS      32
#define RTOS_CFG_EVT_BITS       16
#define RTOS_CFG_STACK_CHECK    2
#define RTOS_CFG_STACK_WARN     1
#define RTOS_CFG_IDLE_SLEEP     1
#define RTOS_CFG_ASSERT         1
#define RTOS_CFG_SW_TIMERS      1
#define RTOS_CFG_RUNTIME_STATS  1
#define RTOS_CFG_EVGROUPS       0
#define RTOS_CFG_SUSPEND        1
#define RTOS_CFG_HEAP           0

#endif  /* RTOS_CONFIG_INCLUDED */
