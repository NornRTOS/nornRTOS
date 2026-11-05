# nornWIFI: join the access point of nw_secrets.h, DHCP, ping the gateway (board only).
# Against the binary SDK (this directory as examples/wifi of the SDK): make BOARD=... PROFILE=...
NORN_SDK ?= ../..
ifeq ($(wildcard $(NORN_SDK)/norn.mk),)
  $(error no SDK at $(NORN_SDK): in the nornRTOS source tree build with "make APP=wifi ..." from the top directory)
endif
APP      = wifi
BOARD   ?= esp32
PROFILE ?= default
MODULES  = nornNET nornWIFI
SRC      = wifi_join.c
include $(NORN_SDK)/norn.mk
