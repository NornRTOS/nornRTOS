# nornRTOS: the modules as one tree. Builds an application from the module sources with
# the application's own configuration, and runs the test suites.
#
#   make APP=hello BOARD=esp32                   image of examples/hello
#   make APP=hello BOARD=esp32 sim | upload | monitor   QEMU/simavr, flash, serial
#   make APP=hello APPDIR=/path/to/app           an application kept elsewhere
#   make test                                    nornCore's suite (host, simavr, QEMU, boards present)
#   make clean
#
# Boards: esp32, mega2560, uno (boards/). An application is a directory with its sources,
# an app.mk (BOARD, MODULES, what runs from flash) and its rtos_config.h, plus nn_config.h
# and nw_config.h for the modules it uses. Everything below is done by the nornCore
# Makefile with APPDIR and NORN_RTOS set to this tree; PORT, EXTRA_CFLAGS, SIM_SECONDS,
# CPU_MHZ and the tool locations pass through.

APP     ?= hello
APPDIR  ?= $(abspath examples/$(APP))
BOARD   ?= esp32
NORN_RTOS = $(abspath .)
CORE      = modules/nornCore
PASS      = APP=$(APP) APPDIR=$(APPDIR) BOARD=$(BOARD) NORN_RTOS=$(NORN_RTOS) \
            $(if $(PORT),PORT=$(PORT)) $(if $(EXTRA_CFLAGS),EXTRA_CFLAGS="$(EXTRA_CFLAGS)") \
            $(if $(SIM_SECONDS),SIM_SECONDS=$(SIM_SECONDS)) $(if $(CPU_MHZ),CPU_MHZ=$(CPU_MHZ))

.PHONY: all image sim upload monitor test clean

all: image

image sim upload monitor:
	@test -d "$(APPDIR)" || { echo "no application at $(APPDIR)"; exit 1; }
	@test -f "$(APPDIR)/rtos_config.h" || { echo "$(APPDIR) has no rtos_config.h (copy modules/nornCore/config/rtos_config.template.h)"; exit 1; }
	$(MAKE) --no-print-directory -C $(CORE) $@ $(PASS)

test:
	$(MAKE) --no-print-directory -C $(CORE) test-all

clean:
	rm -rf build $(CORE)/build modules/nornNET/build modules/nornWIFI/build modules/nornSHELL/build modules/nornStore/build examples/*/build
