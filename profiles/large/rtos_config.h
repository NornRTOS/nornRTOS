#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos_config.h
 * nornRTOS configuration profile "large": the kernel configuration of nornCore for every
 * board, one section per board. A profile is frozen: the libraries of the binary SDK are
 * built from it, a source build compiles the modules with it, and an application
 * compiles against this very file; rtos_init checks that both sides agree.
 *
 * Everything the kernel has, for development and diagnosis: the most tasks, tracing (the
 * rtos_traceHook is called at every switch and wake-up; weak default, hal/trace.h has the
 * recorder) and, on the ESP32, tickless idle. Not for the UNO.
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
#define RTOS_CFG_TICKLESS       1
#define RTOS_CFG_TRACE          1
#define RTOS_CFG_PANIC_ACTION   0
#define RTOS_CFG_NUM_TASKS      31
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
#define RTOS_PRIO_WIFI_RADIO    31
#define RTOS_PRIO_WIFI_SERVICE  30
#define RTOS_PRIO_NET           29
#define RTOS_PRIO_APP_MAX       28
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)   /* ---- mega2560 ---- */
#define RTOS_CFG_TICK_HZ        1000ul
#define RTOS_CFG_IDLE_SLEEP     1
#define RTOS_CFG_SW_TIMERS      1
#define RTOS_CFG_NOTIFY         1
#define RTOS_CFG_TIMER_TASK_PRIO 0
#define RTOS_CFG_TICKLESS       0
#define RTOS_CFG_TRACE          1
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
# error the "large" profile is not offered for the UNO (2 KB of RAM): use default or small
#else
# error unknown board for this profile
#endif

#endif  /* RTOS_CONFIG_INCLUDED */
