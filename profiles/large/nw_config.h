#ifndef NW_CONFIG_INCLUDED
#define NW_CONFIG_INCLUDED
/* nornWIFI in the nornRTOS profile "large" (esp32 only). The task priorities are the kernel
   profile's RTOS_PRIO_WIFI_RADIO and RTOS_PRIO_WIFI_SERVICE; the defaults of nw_config.h
   otherwise: HT20, reconnection on, the normal buffer profile. */
#if !defined(__XTENSA__)
# error nornWIFI runs on the esp32 only
#endif

#endif  /* NW_CONFIG_INCLUDED */
