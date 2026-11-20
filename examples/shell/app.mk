# nosh, the shell, on the console UART beside a blinking task. From the nornRTOS directory:
#   make APP=shell BOARD=esp32 sim      (or mega2560 in simavr; type at the terminal)
#   make APP=shell BOARD=esp32 upload && make APP=shell BOARD=esp32 monitor
# SHELL_WITHOUT_NOSH=1 builds the same application without the shell: the binary must
# then be free of every nosh symbol (tools/size_check.sh).
BOARD ?= esp32
MODULES += $(NORN_RTOS)/modules/nornSHELL/nosh.mk
ifeq ($(SHELL_WITHOUT_NOSH),1)
  MOD_CFLAGS += -DSHELL_WITHOUT_NOSH
endif
