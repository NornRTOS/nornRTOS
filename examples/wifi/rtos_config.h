#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the wifi example. nornWIFI's nw02 configuration: radio task 5, WiFi service 4, net task 3, the example's task 1; 32 event bits, the heap and event groups are what the radio adapter needs.
 */
#define RTOS_CFG_NUM_TASKS      5
#define RTOS_CFG_EVT_BITS       32
#define RTOS_CFG_STACK_CHECK    2
#define RTOS_CFG_HEAP           1
#define RTOS_CFG_HAL_HOOKS      1       /* panics and stack warnings on the console, hal/hooks.c */
/* Everything not set here is the kernel's default (kernel/rtos_types.h). */

#endif  /* RTOS_CONFIG_INCLUDED */
