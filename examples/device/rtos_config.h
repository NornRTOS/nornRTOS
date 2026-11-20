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
#define RTOS_CFG_EVT_BITS       32
#define RTOS_CFG_STACK_CHECK    2
#define RTOS_CFG_HEAP           1
#define RTOS_CFG_HAL_HOOKS      1       /* panics and stack warnings on the console, hal/hooks.c */
#ifndef RTOS_CFG_SMP
# define RTOS_CFG_SMP           1       /* one kernel on both cores; nornWIFI stays on core 0 */
#endif
#if RTOS_CFG_SMP
# define RTOS_CFG_SMP_AFFINITY  1
#endif
/* Everything not set here is the kernel's default (kernel/rtos_types.h). */

#endif  /* RTOS_CONFIG_INCLUDED */
