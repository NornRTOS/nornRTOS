# nornRTOS: the modules as one tree. Builds an application from the module
# sources against a configuration profile, builds the per profile libraries and the
# binary SDK, and runs the test suites.
#
#   make APP=hello BOARD=esp32 PROFILE=default          image of examples/hello
#   make APP=hello BOARD=esp32 sim | upload | monitor    QEMU/simavr, flash, serial
#   make APP=hello APPDIR=/path/to/app                   an application kept elsewhere
#   make lib BOARD=esp32 PROFILE=default                 libnornCore, libnornNET, libnornWIFI of the profile
#   make sdk                                             the binary SDK bundle in build/sdk (tools/sdk.sh)
#   make test                                            nornCore's suite (host, simavr, QEMU, boards present)
#   make clean
#
# Boards: esp32, mega2560, uno (boards/). Profiles: small, default, large (profiles/): an
# application without an rtos_config.h of its own compiles against the profile's; the
# modules it names in app.mk take their nn_config.h and nw_config.h from there too.
# Everything below is done by the nornCore Makefile with APPDIR, PROFILE_DIR and NORN_RTOS
# set to this tree; PORT, EXTRA_CFLAGS, SIM_SECONDS, CPU_MHZ and the tool locations pass
# through.

APP     ?= hello
APPDIR  ?= $(abspath examples/$(APP))
BOARD   ?= esp32
PROFILE ?= default
NORN_RTOS = $(abspath .)
CORE      = modules/nornCore
PASS      = APP=$(APP) APPDIR=$(APPDIR) BOARD=$(BOARD) PROFILE=$(PROFILE) \
            PROFILE_DIR=$(NORN_RTOS)/profiles/$(PROFILE) NORN_RTOS=$(NORN_RTOS) \
            $(if $(PORT),PORT=$(PORT)) $(if $(EXTRA_CFLAGS),EXTRA_CFLAGS="$(EXTRA_CFLAGS)") \
            $(if $(SIM_SECONDS),SIM_SECONDS=$(SIM_SECONDS)) $(if $(CPU_MHZ),CPU_MHZ=$(CPU_MHZ))

.PHONY: all image sim upload monitor lib sdk test clean

all: image

image sim upload monitor:
	@test -d "$(APPDIR)" || { echo "no application at $(APPDIR)"; exit 1; }
	$(MAKE) --no-print-directory -C $(CORE) $@ $(PASS)

lib:
	$(MAKE) --no-print-directory -C $(CORE) lib BOARD=$(BOARD) PROFILE=$(PROFILE) PROFILE_DIR=$(NORN_RTOS)/profiles/$(PROFILE) NORN_RTOS=$(NORN_RTOS)
	@grep -q "nornNET is not offered" profiles/$(PROFILE)/nn_config.h && echo "nornNET: not on this profile/board" || \
	  $(MAKE) --no-print-directory -C modules/nornNET lib BOARD=$(BOARD) PROFILE=$(PROFILE) PROFILE_DIR=$(NORN_RTOS)/profiles/$(PROFILE) NORN_RTOS=$(NORN_RTOS) 2>&1 | grep -v "^make" | tail -1
	@if [ "$(BOARD)" = esp32 ] && ! grep -q "# error nornWIFI needs" profiles/$(PROFILE)/nw_config.h; then \
	  $(MAKE) --no-print-directory -C modules/nornWIFI lib PROFILE=$(PROFILE) PROFILE_DIR=$(NORN_RTOS)/profiles/$(PROFILE) NORN_RTOS=$(NORN_RTOS) 2>&1 | grep -v "^make" | tail -1; fi

sdk:
	tools/sdk.sh

test:
	$(MAKE) --no-print-directory -C $(CORE) test-all

clean:
	rm -rf build $(CORE)/build modules/nornNET/build modules/nornWIFI/build examples/*/build
