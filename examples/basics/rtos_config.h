#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the basics example. nornCore's tc01 configuration; the idle hook is the function rtos_idleHook.
 */
#define RTOS_CFG_NUM_TASKS      3
#define RTOS_CFG_TICK_HZ        1000ul
#define RTOS_CFG_TICK_BITS      32
#define RTOS_CFG_EVT_BITS       16
#define RTOS_CFG_STACK_CHECK    1
#define RTOS_CFG_IDLE_SLEEP     1
#define RTOS_CFG_ASSERT         1
#endif  /* RTOS_CONFIG_INCLUDED */
