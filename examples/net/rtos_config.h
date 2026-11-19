#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the net example. nornNET's nn02 configuration: the net task is 3, the example's task 1.
 */
#define RTOS_CFG_NUM_TASKS      3
#define RTOS_CFG_STACK_CHECK    2
#define RTOS_CFG_HAL_HOOKS      1       /* panics and stack warnings on the console, hal/hooks.c */
/* Everything not set here is the kernel's default (kernel/rtos_types.h). */

#endif  /* RTOS_CONFIG_INCLUDED */
