# Two tasks, a semaphore and a software timer; the hooks as functions.
# Against the binary SDK (this directory as examples/semaphore of the SDK): make BOARD=... PROFILE=...
NORN_SDK ?= ../..
ifeq ($(wildcard $(NORN_SDK)/norn.mk),)
  $(error no SDK at $(NORN_SDK): in the nornRTOS source tree build with "make APP=semaphore ..." from the top directory)
endif
APP      = semaphore
BOARD   ?= esp32
PROFILE ?= default
MODULES  = 
SRC      = semaphore_lib.c
include $(NORN_SDK)/norn.mk
