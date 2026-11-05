#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * Kernel configuration of the bench example. nornCore's tc10 configuration: the defaults an application gets, each overridable.
 */
#define RTOS_CFG_NUM_TASKS      3
#define RTOS_CFG_TICK_HZ        1000ul
#define RTOS_CFG_TICK_BITS      32
#define RTOS_CFG_EVT_BITS       16
#define RTOS_CFG_IDLE_SLEEP     1
#ifndef RTOS_CFG_ASSERT
# define RTOS_CFG_ASSERT        1
#endif
/* Each option can be overridden from EXTRA_CFLAGS to measure its cost alone, as in tc10. */
#if defined(BENCH_LEAN) && BENCH_LEAN
# define BENCH_DEFAULT  0
#else
# define BENCH_DEFAULT  1
#endif
#ifndef RTOS_CFG_STACK_CHECK
# define RTOS_CFG_STACK_CHECK    BENCH_DEFAULT
#endif
#ifndef RTOS_CFG_RUNTIME_STATS
# define RTOS_CFG_RUNTIME_STATS  BENCH_DEFAULT
#endif
#ifndef RTOS_CFG_SW_TIMERS
# define RTOS_CFG_SW_TIMERS      BENCH_DEFAULT
#endif
#ifndef RTOS_CFG_EVGROUPS
# define RTOS_CFG_EVGROUPS       BENCH_DEFAULT
#endif
#ifndef RTOS_CFG_SUSPEND
# define RTOS_CFG_SUSPEND        BENCH_DEFAULT
#endif
#endif  /* RTOS_CONFIG_INCLUDED */
