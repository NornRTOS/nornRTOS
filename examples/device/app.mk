# device: every module of nornRTOS in one application. nornStore keeps the configuration
# (Wi-Fi network, server address, hostname, report interval), nornWIFI joins the network,
# nornNET gets an address by DHCP and carries TCP, nornHTTP posts a JSON report to
# nornHTTP's test server (modules/nornHTTP/tools/demo_server.py) every interval, and nosh
# is on the console with the default commands, `config`, `report` and `net`. ESP32 only
# (the radio); run from the nornRTOS repository root, see README.md.
BOARD ?= esp32
ONLY_BOARDS = esp32
NW_BOARD_ONLY = 1
NORNNET_DIR = $(NORN_RTOS)/modules/nornNET
MODULES += $(NORNNET_DIR)/nornnet.mk
NORNWIF_DIR = $(NORN_RTOS)/modules/nornWIFI
MODULES += $(NORNWIF_DIR)/nornwif.mk
MODULES += $(NORN_RTOS)/modules/nornSHELL/nosh.mk
MODULES += $(NORN_RTOS)/modules/nornStore/nornstore.mk

# nornHTTP is plain C; include its core and the optional nornNET transport.
NORNHTTP_DIR = $(NORN_RTOS)/modules/nornHTTP
MOD_SRC += $(wildcard $(NORNHTTP_DIR)/src/*.c) $(NORNHTTP_DIR)/adapters/norn_net_transport.c
MOD_INC += -I$(NORNHTTP_DIR)/include -I$(NORNHTTP_DIR)/adapters

# Code that may run from flash through the cache. nornStore's store.o and store_crc.o and
# the HAL's flash.o stay in IRAM on purpose: store_save switches the cache off while the
# ROM writes the flash, and the code that does that cannot itself live in flash. The
# `config` command and nosh are fine there: they call into IRAM and get the cache back
# before the return.
FLASH_OBJS += *nn_*.o *norn_http*.o *norn_net_transport.o *nosh*.o *store_cmd_config.o *libgcc.a *libc.a
