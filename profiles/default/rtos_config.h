#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * nornRTOS configuration profile "default": the kernel configuration of nornCore for every
 * board, one section per board. A profile is frozen: the libraries of the binary SDK are
 * built from it, a source build compiles the modules with it, and an application
 * compiles against this very file; rtos_init checks that both sides agree.
 *
 * Everything an application normally wants: statistics, assertions, the stack canary and
 * monitor, event groups, suspend; on the ESP32 also the heap, 32 event bits and the
 * watchdog, as nornWIFI needs them. Start here.
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
#define RTOS_CFG_NUM_TASKS      15
#define RTOS_CFG_TICK_BITS      32
#define RTOS_CFG_EVT_BITS       32
#define RTOS_CFG_STACK_CHECK    2
#define RTOS_CFG_STACK_WARN     1
#define RTOS_CFG_RUNTIME_STATS  1
#define RTOS_CFG_ASSERT         1
#define RTOS_CFG_EVGROUPS       1
#define RTOS_CFG_SUSPEND        1
#define RTOS_CFG_HEAP           1
#define RTOS_CFG_WATCHDOG       1
#define RTOS_PRIO_WIFI_RADIO    15
#define RTOS_PRIO_WIFI_SERVICE  14
#define RTOS_PRIO_NET           13
#define RTOS_PRIO_APP_MAX       12
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)   /* ---- mega2560 ---- */
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
#define RTOS_CFG_STACK_CHECK    2
#define RTOS_CFG_STACK_WARN     1
#define RTOS_CFG_RUNTIME_STATS  1
#define RTOS_CFG_ASSERT         1
#define RTOS_CFG_EVGROUPS       1
#define RTOS_CFG_SUSPEND        1
#define RTOS_CFG_HEAP           0
#define RTOS_CFG_WATCHDOG       0
#define RTOS_PRIO_NET           7
#define RTOS_PRIO_APP_MAX       6
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)    /* ---- uno ---- */
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
#define RTOS_CFG_ASSERT         1
#define RTOS_CFG_EVGROUPS       0
#define RTOS_CFG_SUSPEND        0
#define RTOS_CFG_HEAP           0
#define RTOS_CFG_WATCHDOG       0
#define RTOS_PRIO_NET           4
#define RTOS_PRIO_APP_MAX       3
#else
# error unknown board for this profile
#endif

#endif  /* RTOS_CONFIG_INCLUDED */
