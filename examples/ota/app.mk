# Over the air update over Wi-Fi: fetch an image from the host and hand it to hal/ota.h.
# Build with OTA=1 (the selector and the slots), from the nornRTOS repository root:
#   make APP=ota BOARD=esp32 OTA=1 upload PORT=/dev/ttyUSB0
BOARD ?= esp32
ONLY_BOARDS = esp32
NW_BOARD_ONLY = 1
NO_QEMU = 1
NORNNET_DIR = $(NORN_RTOS)/modules/nornNET
MODULES += $(NORNNET_DIR)/nornnet.mk
NORNWIF_DIR = $(NORN_RTOS)/modules/nornWIFI
MODULES += $(NORNWIF_DIR)/nornwif.mk
NORNHTTP_DIR = $(NORN_RTOS)/modules/nornHTTP
MOD_SRC += $(wildcard $(NORNHTTP_DIR)/src/*.c) $(NORNHTTP_DIR)/adapters/norn_net_transport.c
MOD_INC += -I$(NORNHTTP_DIR)/include -I$(NORNHTTP_DIR)/adapters
FLASH_OBJS += *nn_*.o *norn_http*.o *norn_net_transport.o *libgcc.a *libc.a
