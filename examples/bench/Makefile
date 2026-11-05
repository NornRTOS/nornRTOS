# nornCore's tc10: the cycle benchmark (the UNO lacks the free timers).
# Against the binary SDK (this directory as examples/bench of the SDK): make BOARD=... PROFILE=...
NORN_SDK ?= ../..
ifeq ($(wildcard $(NORN_SDK)/norn.mk),)
  $(error no SDK at $(NORN_SDK): in the nornRTOS source tree build with "make APP=bench ..." from the top directory)
endif
APP      = bench
BOARD   ?= esp32
PROFILE ?= default
MODULES  = 
SRC      = bench_bench.c
include $(NORN_SDK)/norn.mk
