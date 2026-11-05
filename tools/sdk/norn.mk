# nornSDK: build an application against the nornCore library of a configuration profile.
# An application Makefile sets a few variables and includes this file:
#
#     NORN_SDK = ../..            where this SDK is
#     APP      = blink            name of the image
#     BOARD   ?= esp32            esp32, mega2560 or uno
#     PROFILE ?= default          small, default or large (profiles/<name>/, one file per module)
#     SRC      = blink.c          the application's sources
#     include $(NORN_SDK)/norn.mk
#
# Targets: all (the image), sim (QEMU for esp32, simavr for the AVR boards), upload
# (PORT=...), monitor, clean. The application has no rtos_config.h: the profile's is
# used, and rtos_init checks at start that the library was built from the same one.
# The hooks are functions the application may define: rtos_idleHook, rtos_stackWarnHook,
# rtos_panicHook, rtos_assertFailed, rtos_traceHook.
#
# Tool locations (override on the command line or in the environment): XTENSA_ROOT,
# ESP_PYTHON (a Python with esptool), QEMU_ESP, and simavr/avr-gcc/avrdude on the PATH.

NORN_SDK ?= .
BOARD    ?= esp32
PROFILE  ?= default
BUILD    ?= build/$(BOARD)-$(PROFILE)
TARGET    = $(BUILD)/$(APP)

ARCH_esp32    = xtensa
ARCH_mega2560 = avr
ARCH_uno      = avr
ARCH = $(ARCH_$(BOARD))
ifeq ($(ARCH),)
  $(error unknown BOARD '$(BOARD)': esp32, mega2560 or uno)
endif

INC_DIR  = $(NORN_SDK)/include/nornCore
PROFILE_DIR = $(NORN_SDK)/profiles/$(PROFILE)
LIB      = $(NORN_SDK)/lib/$(BOARD)/$(PROFILE)/libnornCore.a
ifeq ($(wildcard $(LIB)),)
  $(error no profile '$(PROFILE)' for $(BOARD): see $(NORN_SDK)/lib/$(BOARD)/)
endif
INC = -I$(PROFILE_DIR) -I$(INC_DIR) -I$(INC_DIR)/port/$(ARCH) -I$(INC_DIR)/hal -I$(INC_DIR)/hal/$(ARCH) $(EXTRA_INC)
OBJ = $(patsubst %.c,$(BUILD)/%.o,$(SRC))

# Modules: MODULES = nornNET nornWIFI in the application Makefile includes
# modules/<name>.mk, which adds the module's include path, its library of the same
# profile, its linker fragments and what of it runs from flash (MOD_INC, MOD_CFLAGS,
# MOD_LIBS, MOD_LD, FLASH_OBJS and friends).
MODULES ?=
MOD_INC    =
MOD_CFLAGS =
MOD_LIBS   =
MOD_LDLIBS =
MOD_LD     =
include $(patsubst %,$(NORN_SDK)/modules/%.mk,$(MODULES))
INC += $(MOD_INC)

# ---- esp32 ------------------------------------------------------------------------------
ifeq ($(ARCH),xtensa)
CPU_MHZ     ?= 240
F_CPU        = $(CPU_MHZ)000000UL
APB_HZ       = $(if $(filter 40,$(CPU_MHZ)),40000000UL,80000000UL)
PORT        ?= /dev/ttyUSB0
XTENSA_ROOT ?= $(HOME)/dev/tools/xtensa-esp-elf-16.1.0
ESP_PYTHON  ?= $(HOME)/dev/tools/esp-venv/bin/python
ESPTOOL     ?= $(ESP_PYTHON) -m esptool
QEMU_ESP    ?= $(HOME)/dev/tools/qemu-esp
FLASH_BASE  ?= 0x80000
CC      = $(XTENSA_ROOT)/bin/xtensa-esp32-elf-gcc
SIZE    = $(XTENSA_ROOT)/bin/xtensa-esp32-elf-size
OBJCOPY = $(XTENSA_ROOT)/bin/xtensa-esp32-elf-objcopy
LDS     = $(BUILD)/esp32.ld
LDDIR   = $(NORN_SDK)/linker/esp32
CFLAGS  = -std=c99 -Wall -Wextra -Os -g -mlongcalls -DF_CPU=$(F_CPU) -DHAL_APB_HZ=$(APB_HZ) $(INC) $(MOD_CFLAGS) \
          -ffunction-sections -fdata-sections -fno-common -MMD -MP $(EXTRA_CFLAGS)
LDFLAGS = -nostartfiles -nostdlib -Wl,--gc-sections -Wl,-T,$(LDS) -Wl,-Map,$(TARGET).map
LDLIBS  = -lc -lgcc

all: $(TARGET).bin $(TARGET).flash.bin
	$(SIZE) $(TARGET).elf

# The linker script: memory layout, flash window, section rules, ROM symbols, and the
# lists of what the application puts in flash (FLASH_OBJS: object or archive name
# patterns, FLASH_SECTIONS: input section names).
# FLASH_OBJS entries: an object name pattern (*blink.o), an archive (*libnornNET.a: every
# member) or a member of one (*libnornWIFI.a:nw_wifi.o).
$(LDS): $(LDDIR)/mem_single.ld $(LDDIR)/mem_flash.ld $(LDDIR)/esp32.ld $(LDDIR)/esp32_rom.ld $(MOD_LD) $(MAKEFILE_LIST)
	@mkdir -p $(BUILD)
	cat $(LDDIR)/mem_single.ld > $@
	sed 's/@FLASH_BASE@/$(FLASH_BASE)/g' $(LDDIR)/mem_flash.ld >> $@
	cat $(LDDIR)/esp32.ld $(LDDIR)/esp32_rom.ld $(MOD_LD) | sed "s|@LD_DIR@|$(abspath $(BUILD))|g" >> $@
	@: > $(BUILD)/ld_flash_text.ld; for p in $(FLASH_OBJS); do case $$p in *:*) ;; *.a) p="$$p:";; esac; echo "$$p(.literal .literal.* .text .text.*)" >> $(BUILD)/ld_flash_text.ld; done
	@for p in $(FLASH_SECTIONS); do echo "*($$p)" >> $(BUILD)/ld_flash_text.ld; done
	@: > $(BUILD)/ld_flash_rodata.ld; for p in $(FLASH_OBJS); do case $$p in *:*) ;; *.a) p="$$p:";; esac; echo "$$p(.rodata .rodata.* .srodata .srodata.*)" >> $(BUILD)/ld_flash_rodata.ld; done
	@for p in $(FLASH_RODATA_SECTIONS); do echo "*($$p)" >> $(BUILD)/ld_flash_rodata.ld; done
	@: > $(BUILD)/ld_iram.ld; for p in $(IRAM_SECTIONS); do echo "*($$p)" >> $(BUILD)/ld_iram.ld; done
	@: > $(BUILD)/ld_dram.ld; for p in $(DRAM_SECTIONS); do echo "*($$p)" >> $(BUILD)/ld_dram.ld; done

$(TARGET).elf: $(OBJ) $(LIB) $(MOD_LIBS) $(LDS)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) -Wl,--whole-archive $(LIB) -Wl,--no-whole-archive $(MOD_LDLIBS) $(LDLIBS)

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -R .flash.rodata -R .flash.text $< $(TARGET).ram.elf
	$(ESPTOOL) --chip esp32 elf2image -fm dio -ff 40m -fs 4MB -o $@ $(TARGET).ram.elf

$(TARGET).flash.bin: $(TARGET).elf
	$(OBJCOPY) -O binary -j .flash.rodata -j .flash.text $< $@

sim: $(TARGET).bin $(TARGET).flash.bin
	QEMU_ESP=$(QEMU_ESP) $(ESP_PYTHON) $(NORN_SDK)/tools/qemu_run.py $< $(or $(SIM_SECONDS),30) $(TARGET).flash.bin@$(FLASH_BASE)

upload: $(TARGET).bin $(TARGET).flash.bin
	$(ESPTOOL) --chip esp32 --port $(PORT) --baud 460800 write-flash 0x1000 $< $(FLASH_BASE) $(TARGET).flash.bin

monitor:
	$(ESP_PYTHON) $(NORN_SDK)/tools/monitor.py $(PORT) 115200
endif

# ---- avr --------------------------------------------------------------------------------
ifeq ($(ARCH),avr)
MCU_mega2560 = atmega2560
MCU_uno      = atmega328p
MCU     = $(MCU_$(BOARD))
F_CPU   = 16000000UL
PORT   ?= /dev/ttyACM0
AVRDUDE_PROG_mega2560 = -c wiring -b 115200 -D
AVRDUDE_PROG_uno      = -c arduino -b 115200
CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size
CFLAGS  = -std=c99 -Wall -Wextra -Os -g -mmcu=$(MCU) -DF_CPU=$(F_CPU) $(INC) \
          -ffunction-sections -fdata-sections -fno-common -MMD -MP $(EXTRA_CFLAGS)
LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections,--relax -Wl,-Map,$(TARGET).map

all: $(TARGET).hex
	$(SIZE) --format=avr --mcu=$(MCU) $(TARGET).elf

$(TARGET).elf: $(OBJ) $(LIB) $(MOD_LIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) -Wl,--whole-archive $(LIB) -Wl,--no-whole-archive $(MOD_LDLIBS)

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

sim: $(TARGET).elf
	simavr -m $(MCU) -f $(F_CPU:UL=) $<

upload: $(TARGET).hex
	avrdude -p $(MCU) $(AVRDUDE_PROG_$(BOARD)) -P $(PORT) -U flash:w:$<:i

monitor:
	python3 $(NORN_SDK)/tools/monitor.py $(PORT) 115200
endif

# ---- common -----------------------------------------------------------------------------
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build

.PHONY: all sim upload monitor clean
-include $(OBJ:.o=.d)
