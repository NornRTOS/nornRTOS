#ifndef NN_CONFIG_INCLUDED
#define NN_CONFIG_INCLUDED
/* nornNET in the device example. The net task takes its default priority, 3: below the
   two Wi-Fi tasks (4, 5), above the application (2) and nosh (1). */
#define NN_CFG_NUM_BUFS    12
#define NN_CFG_TASK_STACK  3072
#endif
