#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the timing example. nornCore's tc04 configuration.
 */
#define RTOS_CFG_NUM_TASKS      3
#define RTOS_CFG_HAL_HOOKS      1       /* panics and stack warnings on the console, hal/hooks.c */
/* Everything not set here is the kernel's default (kernel/rtos_types.h). */

#endif  /* RTOS_CONFIG_INCLUDED */
