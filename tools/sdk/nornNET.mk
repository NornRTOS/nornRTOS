# nornNET in an SDK application: MODULES += nornNET. The library of the board's profile,
# its headers and its profile configuration (nn_config.h, which the library was built
# with and the application must see too). Everything of the stack runs from flash on the
# ESP32: nothing of it is called from an interrupt.
NN_LIB = $(NORN_SDK)/lib/$(BOARD)/$(PROFILE)/libnornNET.a
ifeq ($(wildcard $(NN_LIB)),)
  $(error nornNET has no library for $(BOARD)/$(PROFILE))
endif
MOD_INC    += -I$(NORN_SDK)/include/nornNET          # nn_config.h comes from the profile directory
MOD_CFLAGS += -DNN_OS_NORNOS -DNN_CONFIG_H
MOD_LIBS   += $(NN_LIB)
MOD_LDLIBS += $(NN_LIB)
FLASH_OBJS += *libnornNET.a
