#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the device example. Tasks: nosh 1, the application 2, the
 * nornNET task 3 (nn_config.h), the nornWIFI service 4 and radio 5 (the two highest, by
 * nw_config.h's defaults). nornWIFI needs the heap, event groups and 32 event bits; the
 * run time statistics and the stack canary are for `ps`, `mem` and `sched`.
 */

#define RTOS_CFG_NUM_TASKS      5
#define RTOS_CFG_TICK_HZ        1000ul
#define RTOS_CFG_TICK_BITS      32
#define RTOS_CFG_EVT_BITS       32
#define RTOS_CFG_STACK_CHECK    2
#define RTOS_CFG_STACK_WARN     1
#define RTOS_CFG_IDLE_SLEEP     1
#define RTOS_CFG_ASSERT         1
#define RTOS_CFG_SW_TIMERS      1
#define RTOS_CFG_RUNTIME_STATS  1
#define RTOS_CFG_HEAP           1
#define RTOS_CFG_EVGROUPS       1
#define RTOS_CFG_NOTIFY         1
#define RTOS_CFG_SUSPEND        1

#endif  /* RTOS_CONFIG_INCLUDED */
