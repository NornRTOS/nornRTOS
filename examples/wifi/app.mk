# nornWIFI: join the access point of nw_secrets.h, DHCP, ping the gateway (board only).
# Source build from the nornRTOS directory: make APP=wifi BOARD=... PROFILE=... [sim|upload]
BOARD ?= esp32
ONLY_BOARDS = esp32
NW_BOARD_ONLY = 1
NORNNET_DIR = $(NORN_RTOS)/modules/nornNET
MODULES += $(NORNNET_DIR)/nornnet.mk
NORNWIF_DIR = $(NORN_RTOS)/modules/nornWIFI
MODULES += $(NORNWIF_DIR)/nornwif.mk
FLASH_OBJS += *nn_*.o *libgcc.a *libc.a
