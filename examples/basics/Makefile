# nornCore's tc01: every basic primitive, self checking (RESULT line).
# Against the binary SDK (this directory as examples/basics of the SDK): make BOARD=... PROFILE=...
NORN_SDK ?= ../..
ifeq ($(wildcard $(NORN_SDK)/norn.mk),)
  $(error no SDK at $(NORN_SDK): in the nornRTOS source tree build with "make APP=basics ..." from the top directory)
endif
APP      = basics
BOARD   ?= esp32
PROFILE ?= default
MODULES  = 
SRC      = basics_basics.c
include $(NORN_SDK)/norn.mk
