# nornNET over the open_eth MAC: pings QEMU's gateway (QEMU only).
# Against the binary SDK (this directory as examples/net of the SDK): make BOARD=... PROFILE=...
NORN_SDK ?= ../..
ifeq ($(wildcard $(NORN_SDK)/norn.mk),)
  $(error no SDK at $(NORN_SDK): in the nornRTOS source tree build with "make APP=net ..." from the top directory)
endif
APP      = net
BOARD   ?= esp32
PROFILE ?= default
MODULES  = nornNET
SRC      = net_ping.c
include $(NORN_SDK)/norn.mk
