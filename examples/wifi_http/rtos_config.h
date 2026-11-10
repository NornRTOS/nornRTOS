#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/* Application 1, network 3, Wi-Fi service 4, radio 5 (same as examples/wifi). */
#define RTOS_CFG_NUM_TASKS   5
#define RTOS_CFG_TICK_HZ     1000ul
#define RTOS_CFG_TICK_BITS   32
#define RTOS_CFG_EVT_BITS    32
#define RTOS_CFG_STACK_CHECK 2
#define RTOS_CFG_IDLE_SLEEP  1
#define RTOS_CFG_ASSERT      1
#define RTOS_CFG_HEAP        1
#define RTOS_CFG_EVGROUPS    1
#endif
