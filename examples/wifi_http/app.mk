# One HTTP GET over Wi-Fi. Run from the nornRTOS repository root.
BOARD ?= esp32
ONLY_BOARDS = esp32
NW_BOARD_ONLY = 1
NORNNET_DIR = $(NORN_RTOS)/modules/nornNET
MODULES += $(NORNNET_DIR)/nornnet.mk
NORNWIF_DIR = $(NORN_RTOS)/modules/nornWIFI
MODULES += $(NORNWIF_DIR)/nornwif.mk

# nornHTTP is plain C; include its core and the optional nornNET transport.
NORNHTTP_DIR = $(NORN_RTOS)/modules/nornHTTP
MOD_SRC += $(wildcard $(NORNHTTP_DIR)/src/*.c) $(NORNHTTP_DIR)/adapters/norn_net_transport.c
MOD_INC += -I$(NORNHTTP_DIR)/include -I$(NORNHTTP_DIR)/adapters
FLASH_OBJS += *nn_*.o *norn_http*.o *norn_net_transport.o *libgcc.a *libc.a
