#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the semaphore example. Two tasks, a software timer and notifications.
 */
#define RTOS_CFG_NUM_TASKS      2
#define RTOS_CFG_RUNTIME_STATS  0
#define RTOS_CFG_EVGROUPS       0
#define RTOS_CFG_SUSPEND        0
/* Everything not set here is the kernel's default (kernel/rtos_types.h). */

#endif  /* RTOS_CONFIG_INCLUDED */
