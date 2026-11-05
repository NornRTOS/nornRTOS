#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * nornRTOS configuration profile "small": the kernel configuration of nornCore for every
 * board, one section per board. A profile is frozen: the libraries of the binary SDK are
 * built from it, a source build compiles the modules with it, and an application
 * compiles against this very file; rtos_init checks that both sides agree.
 *
 * The lean kernel: the fewest tasks, 16 bit ticks and 8 event bits on the AVR, no
 * statistics, assertions, stack monitor, heap, event groups, suspend or watchdog; nornNET
 * fits on the ESP32, nornWIFI does not. The fastest kernel there is.
 *
 * The priority map at the end of each section places the tasks of the nornRTOS modules at
 * the top (nornWIFI radio and service, nornNET net task) and gives the application
 * 1..RTOS_PRIO_APP_MAX.
 */

#if defined(__XTENSA__)                     /* ---- esp32 ---- */
#define RTOS_CFG_TICK_HZ        1000ul
#define RTOS_CFG_IDLE_SLEEP     1
#define RTOS_CFG_SW_TIMERS      1
#define RTOS_CFG_NOTIFY         1
#define RTOS_CFG_TIMER_TASK_PRIO 0
#define RTOS_CFG_TICKLESS       0
#define RTOS_CFG_TRACE          0
#define RTOS_CFG_PANIC_ACTION   0
#define RTOS_CFG_NUM_TASKS      7
#define RTOS_CFG_TICK_BITS      32
#define RTOS_CFG_EVT_BITS       16
#define RTOS_CFG_STACK_CHECK    1
#define RTOS_CFG_STACK_WARN     0
#define RTOS_CFG_RUNTIME_STATS  0
#define RTOS_CFG_ASSERT         0
#define RTOS_CFG_EVGROUPS       0
#define RTOS_CFG_SUSPEND        0
#define RTOS_CFG_HEAP           0
#define RTOS_CFG_WATCHDOG       0
#define RTOS_PRIO_NET           7
#define RTOS_PRIO_APP_MAX       6
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)   /* ---- mega2560 ---- */
#define RTOS_CFG_TICK_HZ        1000ul
#define RTOS_CFG_IDLE_SLEEP     1
#define RTOS_CFG_SW_TIMERS      1
#define RTOS_CFG_NOTIFY         1
#define RTOS_CFG_TIMER_TASK_PRIO 0
#define RTOS_CFG_TICKLESS       0
#define RTOS_CFG_TRACE          0
#define RTOS_CFG_PANIC_ACTION   0
#define RTOS_CFG_NUM_TASKS      4
#define RTOS_CFG_TICK_BITS      16
#define RTOS_CFG_EVT_BITS       8
#define RTOS_CFG_STACK_CHECK    1
#define RTOS_CFG_STACK_WARN     0
#define RTOS_CFG_RUNTIME_STATS  0
#define RTOS_CFG_ASSERT         0
#define RTOS_CFG_EVGROUPS       0
#define RTOS_CFG_SUSPEND        0
#define RTOS_CFG_HEAP           0
#define RTOS_CFG_WATCHDOG       0
#define RTOS_PRIO_NET           4
#define RTOS_PRIO_APP_MAX       3
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)    /* ---- uno ---- */
#define RTOS_CFG_TICK_HZ        1000ul
#define RTOS_CFG_IDLE_SLEEP     1
#define RTOS_CFG_SW_TIMERS      0
#define RTOS_CFG_NOTIFY         1
#define RTOS_CFG_TIMER_TASK_PRIO 0
#define RTOS_CFG_TICKLESS       0
#define RTOS_CFG_TRACE          0
#define RTOS_CFG_PANIC_ACTION   0
#define RTOS_CFG_NUM_TASKS      3
#define RTOS_CFG_TICK_BITS      16
#define RTOS_CFG_EVT_BITS       8
#define RTOS_CFG_STACK_CHECK    1
#define RTOS_CFG_STACK_WARN     0
#define RTOS_CFG_RUNTIME_STATS  0
#define RTOS_CFG_ASSERT         0
#define RTOS_CFG_EVGROUPS       0
#define RTOS_CFG_SUSPEND        0
#define RTOS_CFG_HEAP           0
#define RTOS_CFG_WATCHDOG       0
#define RTOS_PRIO_APP_MAX       3
#else
# error unknown board for this profile
#endif

#endif  /* RTOS_CONFIG_INCLUDED */
