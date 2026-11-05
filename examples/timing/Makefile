# nornCore's tc04: context switch, interrupt latency, periodic jitter.
# Against the binary SDK (this directory as examples/timing of the SDK): make BOARD=... PROFILE=...
NORN_SDK ?= ../..
ifeq ($(wildcard $(NORN_SDK)/norn.mk),)
  $(error no SDK at $(NORN_SDK): in the nornRTOS source tree build with "make APP=timing ..." from the top directory)
endif
APP      = timing
BOARD   ?= esp32
PROFILE ?= default
MODULES  = 
SRC      = timing_timing.c
include $(NORN_SDK)/norn.mk
