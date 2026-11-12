# nornStore with nosh's `config` command: a configuration that survives a reset and can
# be changed from the console. From the nornRTOS directory:
#   make APP=store BOARD=esp32 sim      (QEMU; modules/nornSHELL/tools/qemu_talk.py --flash
#                                        keeps the flash file between runs, so a second run
#                                        is a reboot that finds the saved configuration)
#   make APP=store BOARD=esp32 upload && make APP=store BOARD=esp32 monitor
#   make APP=store BOARD=mega2560 sim   (simavr emulates the EEPROM but does not keep it)
# STORE_WITHOUT_STORE=1 builds the same application without the store: the binary must
# then be free of every store_ and hal_flash symbol (tools/size_check.sh).
BOARD ?= esp32
MODULES += $(NORN_RTOS)/modules/nornSHELL/nosh.mk
MODULES += $(NORN_RTOS)/modules/nornStore/nornstore.mk
ifeq ($(STORE_WITHOUT_STORE),1)
  MOD_CFLAGS += -DSTORE_WITHOUT_STORE
endif
