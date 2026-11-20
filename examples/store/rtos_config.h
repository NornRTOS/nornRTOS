#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the store example: one task, the default options.
 */

#define RTOS_CFG_NUM_TASKS      2
#define RTOS_CFG_SW_TIMERS      0
#define RTOS_CFG_RUNTIME_STATS  0
#define RTOS_CFG_EVGROUPS       0
#define RTOS_CFG_SUSPEND        0
#define RTOS_CFG_HAL_HOOKS      1       /* panics and stack warnings on the console, hal/hooks.c */
/* Everything not set here is the kernel's default (kernel/rtos_types.h). */

#endif  /* RTOS_CONFIG_INCLUDED */
