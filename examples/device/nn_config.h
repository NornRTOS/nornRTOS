#ifndef NN_CONFIG_INCLUDED
#define NN_CONFIG_INCLUDED
/* nornNET in the device example: the network task at priority 3, between the application
   (2) and the Wi-Fi tasks (4, 5). */
#define NN_CFG_NUM_BUFS    12
#define NN_CFG_TASK_PRIO   3
#define NN_CFG_TASK_STACK  3072
#endif
