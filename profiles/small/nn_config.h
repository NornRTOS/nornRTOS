#ifndef NN_CONFIG_INCLUDED
#define NN_CONFIG_INCLUDED
/* nornNET in the nornRTOS profile "small", one section per board. The net task's priority
   is the kernel profile's RTOS_PRIO_NET. The UNO has no room for the stack. */
#if defined(__XTENSA__)                     /* ---- esp32 ---- */
#define NN_CFG_NUM_BUFS         8u
#define NN_CFG_RX_QUEUE         8u
#define NN_CFG_TASK_STACK       2048u
#define NN_CFG_ARP_ENTRIES      4u
#define NN_CFG_DNS_CACHE        2u
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)   /* ---- mega2560, SLIP ---- */
#define NN_CFG_NUM_BUFS         4u
#define NN_CFG_BUF_DATA         600u
#define NN_CFG_RX_QUEUE         4u
#define NN_CFG_TASK_STACK       512u
#define NN_CFG_ARP_ENTRIES      4u
#define NN_CFG_UDP_RXQ          2u
#define NN_CFG_DNS_QUERIES      1u
#define NN_CFG_DNS_CACHE        2u
#define NN_CFG_TCP_MSS          536u
#else
# error nornNET is not offered for this board
#endif
#endif  /* NN_CONFIG_INCLUDED */
