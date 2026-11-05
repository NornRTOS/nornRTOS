# nornNET over the open_eth MAC: pings QEMU's gateway (QEMU only).
# From the nornRTOS directory: make APP=net BOARD=... [sim|upload]
BOARD ?= esp32
ONLY_BOARDS = esp32
MODULES += $(NORN_RTOS)/modules/nornNET/nornnet.mk
NORNNET_DIR = $(NORN_RTOS)/modules/nornNET
FLASH_OBJS += *nn_*.o *libgcc.a *libc.a
