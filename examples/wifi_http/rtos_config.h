#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/* Application 1, network 3, Wi-Fi service 4, radio 5 (same as examples/wifi). */
#define RTOS_CFG_NUM_TASKS   5
#define RTOS_CFG_EVT_BITS    32
#define RTOS_CFG_STACK_CHECK 2
#define RTOS_CFG_HEAP        1
#define RTOS_CFG_HAL_HOOKS      1       /* panics and stack warnings on the console, hal/hooks.c */
/* Everything not set here is the kernel's default (kernel/rtos_types.h). */

#endif
