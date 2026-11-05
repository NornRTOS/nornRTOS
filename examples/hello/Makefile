# The smallest application: a periodic task printing once a second.
# Against the binary SDK (this directory as examples/hello of the SDK): make BOARD=... PROFILE=...
NORN_SDK ?= ../..
ifeq ($(wildcard $(NORN_SDK)/norn.mk),)
  $(error no SDK at $(NORN_SDK): in the nornRTOS source tree build with "make APP=hello ..." from the top directory)
endif
APP      = hello
BOARD   ?= esp32
PROFILE ?= default
MODULES  = 
SRC      = hello.c
include $(NORN_SDK)/norn.mk
