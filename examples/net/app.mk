# nornNET over the open_eth MAC in QEMU's user mode network: an address by DHCP from
# QEMU's server (10.0.2.2), twenty pings to it, one to our own loopback, then nosh on the
# console with nornNET's `net` command (QEMU only).
#   make APP=net BOARD=esp32 sim           then: net, net dhcp release, net dhcp start, net ping 10.0.2.2
# NET_WITHOUT_NOSH=1 builds the same application without the shell: the binary must then
# be free of every nosh and `net` command symbol (tools/size_check.sh).
BOARD ?= esp32
ONLY_BOARDS = esp32
MODULES += $(NORN_RTOS)/modules/nornNET/nornnet.mk
NORNNET_DIR = $(NORN_RTOS)/modules/nornNET
ifeq ($(NET_WITHOUT_NOSH),1)
  MOD_CFLAGS += -DNET_WITHOUT_NOSH
else
  MODULES += $(NORN_RTOS)/modules/nornSHELL/nosh.mk
endif
FLASH_OBJS += *nn_*.o *nosh*.o *libgcc.a *libc.a
# `make sim` gives QEMU the network device; nornCore's qemu_run.py reads QEMU_NIC.
export QEMU_NIC ?= user,model=open_eth
