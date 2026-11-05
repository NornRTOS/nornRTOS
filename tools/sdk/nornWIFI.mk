# nornWIFI in an SDK application (esp32 only): MODULES += nornNET nornWIFI. The adapter
# library of the profile, the supplicant, Espressif's binary radio libraries with the ROM
# symbols they need, and the placement: the libraries and the non-critical glue in flash,
# the parts Espressif keeps in IRAM and the adapter (nw_osi, nw_netdev) in IRAM.
ifneq ($(BOARD),esp32)
  $(error nornWIFI only runs on BOARD=esp32)
endif
NW_DIR = $(NORN_SDK)/lib/esp32/$(PROFILE)
NW_LIB = $(NW_DIR)/libnornWIFI.a
ifeq ($(wildcard $(NW_LIB)),)
  $(error nornWIFI has no library for profile $(PROFILE))
endif
MOD_INC    += -I$(NORN_SDK)/include/nornWIFI \
              -isystem $(NORN_SDK)/include/nornWIFI/esp -isystem $(NORN_SDK)/include/nornWIFI/esp/esp_private \
              -isystem $(NORN_SDK)/include/nornWIFI/esp/stubs
MOD_CFLAGS += -DNW_NORNOS -DNW_CONFIG_H -DNW_CFG_NORNNET=1
MOD_LIBS   += $(NW_LIB) $(NW_DIR)/libnwsupp.a
MOD_LDLIBS += -Wl,--start-group $(NW_LIB) $(NW_DIR)/libnwsupp.a -L$(NORN_SDK)/lib/esp32/espressif -lnet80211 -lpp -lcore -lphy -lrtc -Wl,--end-group
MOD_LD     += $(NORN_SDK)/linker/esp32/esp32_rom_wifi.ld
FLASH_OBJS += *libnet80211.a *libpp.a *libcore.a *libphy.a *librtc.a *libnwsupp.a \
              *libnornWIFI.a:nw_wifi.o *libnornWIFI.a:nw_phy.o *libnornWIFI.a:nw_printf.o \
              *libnornWIFI.a:nw_regulatory.o *libnornWIFI.a:nw_phy_init_data.o \
              *libgcc.a *libc.a
IRAM_SECTIONS  += .wifi0iram .wifi0iram.* .wifirxiram .wifirxiram.* .wifiextrairam .wifiextrairam.*
FLASH_SECTIONS += .wifislpiram .wifislpiram.* .wifiorslpiram .wifiorslpiram.* \
                  .wifislprxiram .wifislprxiram.* .phyiram .phyiram.* .coexiram .coexiram.* \
                  .coexsleepiram .coexsleepiram.*
FLASH_RODATA_SECTIONS += .rodata_wlog_error .rodata_wlog_error.* .rodata_wlog_warning .rodata_wlog_warning.* \
                         .rodata_wlog_info .rodata_wlog_info.* .rodata_wlog_debug .rodata_wlog_debug.* \
                         .rodata_wlog_verbose .rodata_wlog_verbose.*
